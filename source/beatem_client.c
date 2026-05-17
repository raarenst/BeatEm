
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netdb.h>
#include "crypto.h"
#include "config.h"
#include "protocol.h"
#include "ws.h"

/* Sleep current thread for `ms` milliseconds. Replaces babelThreadSleep. */
static void sleep_ms(uint32_t ms) {
    struct timespec ts = {
        .tv_sec  = ms / 1000,
        .tv_nsec = (long)(ms % 1000) * 1000000L,
    };
    nanosleep(&ts, NULL);
}

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
char error_flag;
uint8_t g_my_public_key[CLIENT_KEY_SIZE];
uint8_t g_my_secret_key[CLIENT_SECRET_SIZE];
uint8_t g_remote_public_key[CLIENT_KEY_SIZE];
char g_server_url[128];

/* Pending-message handoff from main thread (stdin) to send thread.
 * `send_buf_flag` is 1 iff `g_text_buffer` holds a message that hasn't
 * been consumed yet. The mutex protects both; the cond var lets the
 * main thread block (instead of polling) while a previous message is
 * still in flight.
 */
static pthread_mutex_t g_send_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  g_send_cv  = PTHREAD_COND_INITIALIZER;
static char            send_buf_flag;

/* Keys can be regenerated at runtime via `&genkeys`. Both the send and
 * receive threads read them every iteration, so all access goes through
 * this mutex. Each thread snapshots into stack-local buffers at the top
 * of its loop to keep the critical section short.
 */
static pthread_mutex_t g_keys_mtx = PTHREAD_MUTEX_INITIALIZER;

/* Replay-protection counters.
 *   g_send_counter — strictly monotonic, written into the plaintext of
 *     each REAL outgoing packet. Initialized from wall-clock time at the
 *     first send so it survives client restarts (peer's tracker will
 *     accept the higher value).
 *   g_recv_counter — highest counter we've ever accepted from our
 *     configured peer. New packets must carry counter > this value or
 *     they're dropped as replays / out-of-order. 0 = nothing seen yet.
 */
static uint32_t g_send_counter = 0;
static uint32_t g_recv_counter = 0;

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
    pthread_mutex_lock(&g_keys_mtx);
    crypto_keygen(g_my_public_key, g_my_secret_key);
    pthread_mutex_unlock(&g_keys_mtx);

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

    /* Exit the thread as soon as either side reports an error. Prevents
     * the busy-loop and the false "still sending" output that an
     * unconditional `for(;;)` would produce on a broken socket.
     */
    while (error_flag == 0) {

        /* Snapshot the keypair so a concurrent &genkeys can't change
         * mid-seal. Receive thread does the same.
         */
        uint8_t my_pk[CLIENT_KEY_SIZE];
        uint8_t my_sk[CLIENT_SECRET_SIZE];
        pthread_mutex_lock(&g_keys_mtx);
        memcpy(my_pk, g_my_public_key, CLIENT_KEY_SIZE);
        memcpy(my_sk, g_my_secret_key, CLIENT_SECRET_SIZE);
        pthread_mutex_unlock(&g_keys_mtx);

        /* Stage either a real message or a cover packet under the
         * send lock, then release before doing socket I/O. This way
         * the main thread can prepare the next message while we
         * transmit.
         */
        int have_msg = 0;
        pthread_mutex_lock(&g_send_mtx);
        if (send_buf_flag == 1) {
            /* Plaintext layout: [sender_pk (32)][counter (4 BE)][text].
             * Zero-pad so unused text bytes don't leak.
             *
             * Counter is monotonically increasing, clamped to current
             * wall-clock time so it survives client restarts. The
             * recipient drops anything <= its highest seen value.
             */
            uint32_t now = (uint32_t)time(NULL);
            g_send_counter = (now > g_send_counter) ? now : g_send_counter + 1;

            memset(g_send_plaintext, 0, g_plain_size);
            memcpy(g_send_plaintext, my_pk, CLIENT_KEY_SIZE);
            g_send_plaintext[CLIENT_KEY_SIZE + 0] = (uint8_t)(g_send_counter >> 24);
            g_send_plaintext[CLIENT_KEY_SIZE + 1] = (uint8_t)(g_send_counter >> 16);
            g_send_plaintext[CLIENT_KEY_SIZE + 2] = (uint8_t)(g_send_counter >>  8);
            g_send_plaintext[CLIENT_KEY_SIZE + 3] = (uint8_t)(g_send_counter      );
            memcpy(g_send_plaintext + CLIENT_KEY_SIZE + CLIENT_COUNTER_SIZE,
                   g_text_buffer, g_text_size);
            send_buf_flag = 0;
            pthread_cond_signal(&g_send_cv);
            have_msg = 1;
        }
        pthread_mutex_unlock(&g_send_mtx);

        if (have_msg) {
            int sealed = crypto_seal(g_send_buffer,
                                     g_send_plaintext, g_plain_size,
                                     g_remote_public_key,
                                     my_sk);
            if ((size_t)sealed != g_client_packet_size) {
                printf("Crypto seal error: %d\n", sealed);
                error_flag = 1;
            } else {
                res = ws_send_binary(client_sock,
                                     g_send_buffer,
                                     g_client_packet_size, 1);
                if (res < 0) {
                    printf("Client write error: %d\n", res);
                    error_flag = 1;
                }
            }
        } else {
            /* Cover packet — libsodium CSPRNG bytes. Recipients
             * cannot distinguish these from real ciphertext.
             */
            crypto_random_bytes(g_send_buffer, g_client_packet_size);
            res = ws_send_binary(client_sock,
                                 g_send_buffer,
                                 g_client_packet_size, 1);
            if (res < 0) {
                printf("Client write rnd buffer error: %d\n", res);
                error_flag = 1;
            }
        }
        sleep_ms(g_heartbeat_ms);
    }
    return NULL;
}

void *receive_thread_func(void *arg) {
    (void)arg;
    int res;

    /* Exit on error instead of spinning. The previous shape `for(;;) {
     * if (error_flag == 0) ... }` with no sleep at the end would burn
     * 100% CPU once a read failed.
     */
    while (error_flag == 0) {
        res = ws_recv_binary(client_sock,
                             g_recv_buffer,
                             g_server_packet_size);
        if ((size_t)res != g_server_packet_size) {
            printf("Client read error: %d\n", res);
            error_flag = 1;
            break;
        }

        /* Snapshot keys once per broadcast so all 16 slot-decrypt
         * attempts see a consistent pair, even if &genkeys fires.
         */
        uint8_t my_pk[CLIENT_KEY_SIZE];
        uint8_t my_sk[CLIENT_SECRET_SIZE];
        pthread_mutex_lock(&g_keys_mtx);
        memcpy(my_pk, g_my_public_key, CLIENT_KEY_SIZE);
        memcpy(my_sk, g_my_secret_key, CLIENT_SECRET_SIZE);
        pthread_mutex_unlock(&g_keys_mtx);

        for (size_t i = 0; i < g_max_clients; i++) {
            const uint8_t *slot = g_recv_buffer + g_client_packet_size * i;
            int pt_len = crypto_open(g_recv_plaintext,
                                     slot, g_client_packet_size,
                                     g_remote_public_key,
                                     my_sk);
            if ((size_t)pt_len != g_plain_size) {
                continue;
            }
            /* Echo of our own outgoing packet — drop it.
             * crypto_box's shared secret is symmetric in the keypair,
             * so our own slots decrypt successfully too.
             */
            if (memcmp(g_recv_plaintext, my_pk, CLIENT_KEY_SIZE) == 0) {
                continue;
            }
            /* Replay-protection check — see project_protocol_understanding
             * and the wire format diagram in CLAUDE.md.
             */
            if (!proto_counter_check_and_update(g_recv_plaintext, &g_recv_counter)) {
                continue;
            }

            uint8_t *text = g_recv_plaintext + CLIENT_KEY_SIZE + CLIENT_COUNTER_SIZE;
            text[g_text_size - 1] = '\0';
            printf("\n         ---(%s)---\n>>", (char*)text);
            fflush(stdout);
        }
        /* No sleep here: ws_recv_binary already blocks until a full
         * broadcast frame arrives. Adding a sleep would only let stale
         * broadcasts pile up in the TCP buffer.
         */
    }
    return NULL;
}

static int read_handshake(int sock, proto_handshake_t *h) {
    uint8_t buf[PROTO_HANDSHAKE_SIZE];
    int n = ws_recv_binary(sock, buf, PROTO_HANDSHAKE_SIZE);
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
    printf("Usage: %s <my_secret_hex> <my_public_hex> <remote_public_hex> [host[:port]]\n", prog);
    printf("  Each key is %d hex chars (%d bytes).\n",
           CLIENT_KEY_SIZE * 2, CLIENT_KEY_SIZE);
    printf("  Default host is 127.0.0.1; default port is %u.\n", DEFAULT_SERVER_PORT);
}

/* Open a TCP connection to host:port. Returns the connected socket fd
 * on success, -1 on failure. Resolves hostnames via getaddrinfo with
 * AF_UNSPEC so both IPv4 and IPv6 (numeric or DNS) work.
 */
static int tcp_connect(const char *host, int port) {
    char port_str[16];
    snprintf(port_str, sizeof(port_str), "%d", port);

    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port_str, &hints, &res) != 0 || res == NULL) {
        return -1;
    }
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) {
        freeaddrinfo(res);
        return -1;
    }
    if (connect(sock, res->ai_addr, res->ai_addrlen) < 0) {
        close(sock);
        freeaddrinfo(res);
        return -1;
    }
    freeaddrinfo(res);
    return sock;
}

int main(int argc, char *argv[]) {
    int data;
    pthread_t send_thread;
    pthread_t receive_thread;
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
    int server_port = DEFAULT_SERVER_PORT;
    if (argc == 5) {
        strncpy(g_server_url, argv[4], sizeof(g_server_url) - 1);
        g_server_url[sizeof(g_server_url) - 1] = '\0';
    } else {
        strcpy(g_server_url, "127.0.0.1");
    }
    /* Accepted forms:
     *   host              host alone, default port
     *   host:port         IPv4 / DNS with explicit port (single colon)
     *   [v6]              IPv6 literal in brackets, default port
     *   [v6]:port         IPv6 literal in brackets with explicit port
     *   v6literal         IPv6 literal (>=2 colons), default port
     */
    if (g_server_url[0] == '[') {
        char *rbracket = strchr(g_server_url, ']');
        if (!rbracket) {
            printf("Invalid bracketed host in server arg.\n");
            return -1;
        }
        *rbracket = '\0';
        memmove(g_server_url, g_server_url + 1, strlen(g_server_url + 1) + 1);
        const char *tail = rbracket + 1;
        if (*tail == ':') {
            server_port = atoi(tail + 1);
            if (server_port <= 0 || server_port > 65535) {
                printf("Invalid port in server arg.\n");
                return -1;
            }
        } else if (*tail != '\0') {
            printf("Trailing junk after bracketed host.\n");
            return -1;
        }
    } else {
        char *first = strchr(g_server_url, ':');
        char *last  = strrchr(g_server_url, ':');
        if (first && first == last) {
            *first = '\0';
            server_port = atoi(first + 1);
            if (server_port <= 0 || server_port > 65535) {
                printf("Invalid port in server arg.\n");
                return -1;
            }
        }
        /* Multiple colons → bare IPv6 literal, no port; default applies. */
    }

    send_buf_flag = 0;
    error_flag = 0;

    client_sock = tcp_connect(g_server_url, server_port);
    if (client_sock < 0) {
        printf("Could not connect to server %s:%d (%s)\n",
               g_server_url, server_port, strerror(errno));
        return 1;
    }
    printf("-> Connected to server %s:%d!\n", g_server_url, server_port);

    if (ws_client_handshake(client_sock, g_server_url, server_port) != 0) {
        printf("WebSocket handshake failed.\n");
        close(client_sock);
        return 1;
    }
    printf("-> WebSocket upgrade complete.\n");

    if (read_handshake(client_sock, &h) != 0) {
        close(client_sock);
        return 1;
    }
    g_max_clients        = h.max_clients;
    g_heartbeat_ms       = h.heartbeat_ms;
    g_client_packet_size = h.client_packet_size;
    g_server_packet_size = g_max_clients * g_client_packet_size;
    g_plain_size         = g_client_packet_size - CLIENT_NONCE_SIZE - CLIENT_MAC_SIZE;
    g_text_size          = g_plain_size - CLIENT_KEY_SIZE - CLIENT_COUNTER_SIZE;
    printf("-> Handshake received (v=%u).\n", h.version);

    if (allocate_buffers() != 0) {
        close(client_sock);
        return 1;
    }

    print_welcome();

    if (pthread_create(&send_thread, NULL, send_thread_func, &data) != 0 ||
        pthread_create(&receive_thread, NULL, receive_thread_func, &data) != 0) {
        printf("Could not start worker threads.\n");
        close(client_sock);
        return 1;
    }
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

        /* fgets keeps the trailing newline; strip it so it doesn't end
         * up inside the encrypted plaintext and break the recipient's
         * "---(...)---" display.
         */
        size_t len = strlen(user_input);
        if (len > 0 && user_input[len - 1] == '\n') {
            user_input[--len] = '\0';
        }
        if (len == 0) {
            continue;
        }

        if (strcmp(user_input, "&genkeys") == 0) {
            gen_keys();
        } else {
            pthread_mutex_lock(&g_send_mtx);
            /* Block (rather than spin) until the send thread has
             * consumed any previous message. */
            while (send_buf_flag != 0) {
                pthread_cond_wait(&g_send_cv, &g_send_mtx);
            }
            memset(g_text_buffer, 0, g_text_size);
            strncpy((char*)g_text_buffer, user_input, g_text_size - 1);
            send_buf_flag = 1;
            pthread_mutex_unlock(&g_send_mtx);
        }
    }
    free(user_input);
    close(client_sock);
    return 0;
}
