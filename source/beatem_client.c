
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "babelsock.h"
#include "babelthread.h"
#include "babeltime.h"
#include "crypto.h"
#include "config.h"
#include "protocol.h"

#define HEX_KEY_LEN     (CLIENT_KEY_SIZE * 2 + 1)
#define HEX_SECRET_LEN  (CLIENT_SECRET_SIZE * 2 + 1)

/* Sanity bounds on handshake fields.
 */
#define HS_MIN_PACKET_SIZE  (CLIENT_NONCE_SIZE + CLIENT_MAC_SIZE + CLIENT_KEY_SIZE + 1)
#define HS_MAX_PACKET_SIZE  65536u
#define HS_MAX_CLIENTS      1024u
#define HS_MIN_HEARTBEAT_MS 100u
#define HS_MAX_HEARTBEAT_MS 60000u

int client_sock;
char send_buf_flag;
char error_flag;
uint8_t g_my_public_key[CLIENT_KEY_SIZE];
uint8_t g_my_secret_key[CLIENT_SECRET_SIZE];
uint8_t g_remote_public_key[CLIENT_KEY_SIZE];
char g_server_url[128];

/* Negotiated at connect-time via the server's handshake.
 */
static size_t   g_client_packet_size;
static size_t   g_server_packet_size;
static size_t   g_plain_size;
static size_t   g_text_size;
static size_t   g_max_clients;
static uint32_t g_heartbeat_ms;

/* Heap-allocated after the handshake. The "fixed array" convention
 * is intentionally relaxed here so the client can adapt to whatever
 * sizes the server advertises (see CLAUDE.md).
 */
static uint8_t *g_text_buffer    = NULL;
static uint8_t *g_send_buffer    = NULL;
static uint8_t *g_send_plaintext = NULL;
static uint8_t *g_recv_buffer    = NULL;
static uint8_t *g_recv_plaintext = NULL;

static void print_welcome(void) {
    char my_pub_hex[HEX_KEY_LEN];
    char remote_pub_hex[HEX_KEY_LEN];
    crypto_key_to_hex(my_pub_hex, g_my_public_key, CLIENT_KEY_SIZE);
    crypto_key_to_hex(remote_pub_hex, g_remote_public_key, CLIENT_KEY_SIZE);

    printf("==========================================\n");
    printf("           BEATEM CHAT CLIENT\n");
    printf("          For your eyes only!\n");
    printf("==========================================\n");
    printf("My public key:     %s\n", my_pub_hex);
    printf("Remote public key: %s\n", remote_pub_hex);
    printf("Server cadence:    %u ms, %zu-byte slots, max %zu clients\n",
           g_heartbeat_ms, g_client_packet_size, g_max_clients);
    printf("Plaintext budget:  %zu bytes per message\n", g_text_size);
    printf("------------------------------------------\n");
    printf("Commands:\n");
    printf("  &genkeys to regenerate keys (you must re-share new public key out-of-band).\n");
    printf("------------------------------------------\n");
}

static void gen_keys(void) {
    crypto_keygen(g_my_public_key, g_my_secret_key);

    char my_pub_hex[HEX_KEY_LEN];
    char my_sec_hex[HEX_SECRET_LEN];
    crypto_key_to_hex(my_pub_hex, g_my_public_key, CLIENT_KEY_SIZE);
    crypto_key_to_hex(my_sec_hex, g_my_secret_key, CLIENT_SECRET_SIZE);
    printf("\nNew keypair generated.\n");
    printf("  Secret key (keep private): %s\n", my_sec_hex);
    printf("  Public key (share):        %s\n\n", my_pub_hex);
}

void *send_thread_func(void *arg) {
    (void)arg;
    int res;

    for(;;) {
        if (error_flag == 0) {
            if (send_buf_flag == 1) {

                /* Plaintext layout: [sender_pk (32)][text].
                 * Zero-pad so unused text bytes don't leak.
                 */
                memset(g_send_plaintext, 0, g_plain_size);
                memcpy(g_send_plaintext, g_my_public_key, CLIENT_KEY_SIZE);
                memcpy(g_send_plaintext + CLIENT_KEY_SIZE,
                       g_text_buffer, g_text_size);

                int sealed = crypto_seal(g_send_buffer,
                                         g_send_plaintext, g_plain_size,
                                         g_remote_public_key,
                                         g_my_secret_key);
                if ((size_t)sealed != g_client_packet_size) {
                    printf("Crypto seal error: %d\n", sealed);
                    error_flag = 1;
                } else {
                    res = babelSockWriteAll(client_sock,
                                            (char*)g_send_buffer,
                                            g_client_packet_size);
                    if ((size_t)res != g_client_packet_size) {
                        printf("Client write error: %d\n", res);
                        error_flag = 1;
                    }
                }
                send_buf_flag = 0;
            } else {

                /* Random cover packet. crypto_box_open_easy will fail
                 * to authenticate this and every recipient will drop it.
                 * Uses libsodium's CSPRNG so cover packets are statistically
                 * indistinguishable from real ciphertext (rand() without a
                 * seed is deterministic across runs).
                 */
                crypto_random_bytes(g_send_buffer, g_client_packet_size);
                res = babelSockWriteAll(client_sock,
                                        (char*)g_send_buffer,
                                        g_client_packet_size);
                if ((size_t)res != g_client_packet_size) {
                    printf("Client write rnd buffer error: %d\n", res);
                    error_flag = 1;
                }
            }
        }
        babelThreadSleep(g_heartbeat_ms);
    }
    return NULL;
}

void *receive_thread_func(void *arg) {
    (void)arg;
    int res;

    for(;;) {
        if (error_flag == 0) {
            res = babelSockReadAll(client_sock,
                                   (char*)g_recv_buffer,
                                   g_server_packet_size);
            if ((size_t)res != g_server_packet_size) {
                printf("Client read error: %d\n", res);
                error_flag = 1;
                continue;
            }

            for (size_t i = 0; i < g_max_clients; i++) {
                const uint8_t *slot = g_recv_buffer + g_client_packet_size * i;
                int pt_len = crypto_open(g_recv_plaintext,
                                         slot, g_client_packet_size,
                                         g_remote_public_key,
                                         g_my_secret_key);
                if ((size_t)pt_len != g_plain_size) {
                    continue;
                }
                /* Echo of our own outgoing packet — drop it.
                 * crypto_box's shared secret is symmetric in the keypair,
                 * so our own slots decrypt successfully too.
                 */
                if (memcmp(g_recv_plaintext, g_my_public_key, CLIENT_KEY_SIZE) == 0) {
                    continue;
                }
                /* Real message from the configured remote peer. */
                uint8_t *text = g_recv_plaintext + CLIENT_KEY_SIZE;
                text[g_text_size - 1] = '\0';
                printf("\n         ---(%s)---\n>>", (char*)text);
                fflush(stdout);
            }
        }
        /* No sleep here: babelSockReadAll already blocks for a full
         * broadcast, so a sleep just lets the TCP buffer accumulate
         * stale broadcasts and adds latency to real messages.
         */
    }
    return NULL;
}

static int read_handshake(int sock, proto_handshake_t *h) {
    uint8_t buf[PROTO_HANDSHAKE_SIZE];
    int n = babelSockReadAll(sock, (char*)buf, PROTO_HANDSHAKE_SIZE);
    if (n != PROTO_HANDSHAKE_SIZE) {
        printf("Could not read handshake: %d\n", n);
        return -1;
    }
    if (proto_handshake_decode(buf, h) != 0) {
        printf("Invalid handshake (bad magic or unsupported version).\n");
        return -1;
    }
    if (h->max_clients == 0 || h->max_clients > HS_MAX_CLIENTS) {
        printf("Server advertised unsupported max_clients=%u\n", h->max_clients);
        return -1;
    }
    if (h->heartbeat_ms < HS_MIN_HEARTBEAT_MS ||
        h->heartbeat_ms > HS_MAX_HEARTBEAT_MS) {
        printf("Server advertised unsupported heartbeat_ms=%u\n", h->heartbeat_ms);
        return -1;
    }
    if (h->client_packet_size < HS_MIN_PACKET_SIZE ||
        h->client_packet_size > HS_MAX_PACKET_SIZE) {
        printf("Server advertised unsupported client_packet_size=%u\n",
               h->client_packet_size);
        return -1;
    }
    return 0;
}

static int allocate_buffers(void) {
    g_text_buffer    = calloc(1, g_text_size);
    g_send_buffer    = calloc(1, g_client_packet_size);
    g_send_plaintext = calloc(1, g_plain_size);
    g_recv_buffer    = calloc(1, g_server_packet_size);
    g_recv_plaintext = calloc(1, g_plain_size);
    if (!g_text_buffer || !g_send_buffer || !g_send_plaintext ||
        !g_recv_buffer || !g_recv_plaintext) {
        printf("Could not allocate client buffers.\n");
        return -1;
    }
    return 0;
}

static void usage(const char *prog) {
    printf("Wrong arguments.\n\n");
    printf("Usage: %s <my_secret_hex> <my_public_hex> <remote_public_hex> [server_ip]\n", prog);
    printf("  Each key is %d hex chars (%d bytes).\n",
           CLIENT_KEY_SIZE * 2, CLIENT_KEY_SIZE);
}

int main(int argc, char *argv[]) {
    int res;
    int data;
    BabelThread_t *send_thread;
    BabelThread_t *receive_thread;
    proto_handshake_t h;

    if (crypto_init() != 0) {
        printf("Could not initialize crypto library.\n");
        return 1;
    }

    if (argc != 4 && argc != 5) {
        usage(argv[0]);
        return -1;
    }
    if (crypto_hex_to_key(g_my_secret_key, CLIENT_SECRET_SIZE, argv[1]) != 0) {
        printf("Invalid my_secret_hex.\n");
        return -1;
    }
    if (crypto_hex_to_key(g_my_public_key, CLIENT_KEY_SIZE, argv[2]) != 0) {
        printf("Invalid my_public_hex.\n");
        return -1;
    }
    if (crypto_hex_to_key(g_remote_public_key, CLIENT_KEY_SIZE, argv[3]) != 0) {
        printf("Invalid remote_public_hex.\n");
        return -1;
    }
    if (argc == 5) {
        strncpy(g_server_url, argv[4], sizeof(g_server_url) - 1);
        g_server_url[sizeof(g_server_url) - 1] = '\0';
    } else {
        strcpy(g_server_url, "127.0.0.1");
    }

    send_buf_flag = 0;
    error_flag = 0;

    res = babelSockInit();
    if (res != BABELSOCK_OK) {
        printf("Could not initialize babelsock: %d\n", res);
        return 1;
    }
    client_sock = babelSock(BABELSOCK_TCP);
    if (client_sock < 0) {
        printf("Could not create client socket: %d\n", client_sock);
        return 1;
    }
    res = babelSockConnect(client_sock, g_server_url, SERVER_PORT);
    if (res != BABELSOCK_OK) {
        printf("Could not connect to server: %d\n", res);
        return 1;
    }
    printf("-> Connected to server!\n");

    if (read_handshake(client_sock, &h) != 0) {
        babelSockClose(client_sock);
        return 1;
    }
    g_max_clients        = h.max_clients;
    g_heartbeat_ms       = h.heartbeat_ms;
    g_client_packet_size = h.client_packet_size;
    g_server_packet_size = g_max_clients * g_client_packet_size;
    g_plain_size         = g_client_packet_size - CLIENT_NONCE_SIZE - CLIENT_MAC_SIZE;
    g_text_size          = g_plain_size - CLIENT_KEY_SIZE;
    printf("-> Handshake received (v=%u).\n", h.version);

    if (allocate_buffers() != 0) {
        babelSockClose(client_sock);
        return 1;
    }

    print_welcome();

    babelThreadInit();
    send_thread = babelThreadCreate(send_thread_func, &data, BABELTHREAD_PRIOHINT_LOW);
    babelThreadResume(send_thread);
    receive_thread = babelThreadCreate(receive_thread_func, &data, BABELTHREAD_PRIOHINT_LOW);
    babelThreadResume(receive_thread);
    printf("-> Heartbeat up and running!\n");
    printf("==========================================\n");

    /* user_input is sized to g_text_size so it matches whatever the server
     * negotiated. fgets truncates to size-1 chars + null terminator.
     */
    char *user_input = malloc(g_text_size);
    if (!user_input) {
        printf("Could not allocate input buffer.\n");
        return 1;
    }

    while(1) {
        printf(">>");
        fflush(stdout);
        if (fgets(user_input, (int)g_text_size, stdin) == NULL) {
            break;
        }

        if (user_input[0] != '\n') {
            if (strcmp(user_input, "&genkeys\n") == 0) {
                gen_keys();
            } else {
                while (send_buf_flag != 0) {
                    babelThreadSleep(50);
                }
                memset(g_text_buffer, 0, g_text_size);
                strncpy((char*)g_text_buffer, user_input, g_text_size - 1);
                send_buf_flag = 1;
            }
        }
    }
    free(user_input);
    babelSockClose(client_sock);
    babelSockCleanup();
    return 0;
}
