
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "babelsock.h"
#include "babelthread.h"
#include "babeltime.h"
#include "crypto.h"
#include "config.h"

#define HEX_KEY_LEN (CLIENT_KEY_SIZE * 2 + 1)
#define HEX_SECRET_LEN (CLIENT_SECRET_SIZE * 2 + 1)

int client_sock;
char send_buf_flag;
char error_flag;
char g_text_buffer[CLIENT_TEXT_SIZE];
uint8_t g_my_public_key[CLIENT_KEY_SIZE];
uint8_t g_my_secret_key[CLIENT_SECRET_SIZE];
uint8_t g_remote_public_key[CLIENT_KEY_SIZE];
char g_server_url[128];

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
    uint8_t send_buffer[CLIENT_PACKET_SIZE];
    uint8_t plaintext[CLIENT_PLAIN_SIZE];
    int res;

    for(;;) {
        if (error_flag == 0) {
            if (send_buf_flag == 1) {

                /* Plaintext layout: [sender_pk (32)][text (56)].
                 * Zero-pad so unused text bytes don't leak.
                 */
                memset(plaintext, 0, CLIENT_PLAIN_SIZE);
                memcpy(plaintext, g_my_public_key, CLIENT_KEY_SIZE);
                memcpy(plaintext + CLIENT_KEY_SIZE, g_text_buffer, CLIENT_TEXT_SIZE);

                int sealed = crypto_seal(send_buffer,
                                         plaintext, CLIENT_PLAIN_SIZE,
                                         g_remote_public_key,
                                         g_my_secret_key);
                if (sealed != CLIENT_PACKET_SIZE) {
                    printf("Crypto seal error: %d\n", sealed);
                    error_flag = 1;
                } else {
                    res = babelSockWriteAll(client_sock,
                                            (char*)send_buffer,
                                            CLIENT_PACKET_SIZE);
                    if (res != CLIENT_PACKET_SIZE) {
                        printf("Client write error: %d\n", res);
                        error_flag = 1;
                    }
                }
                send_buf_flag = 0;
            } else {

                /* Random cover packet. crypto_box_open_easy will fail
                 * to authenticate this and every recipient will drop it.
                 */
                for (int idx = 0; idx < CLIENT_PACKET_SIZE; idx++) {
                    send_buffer[idx] = (uint8_t)(rand() & 0xFF);
                }
                res = babelSockWriteAll(client_sock,
                                        (char*)send_buffer,
                                        CLIENT_PACKET_SIZE);
                if (res != CLIENT_PACKET_SIZE) {
                    printf("Client write rnd buffer error: %d\n", res);
                    error_flag = 1;
                }
            }
        }
        babelThreadSleep(CLIENT_SEND_DELAY);
    }
    return NULL;
}

void *receive_thread_func(void *arg) {
    (void)arg;
    uint8_t recvbuf[SERVER_PACKET_SIZE];
    uint8_t plaintext[CLIENT_PLAIN_SIZE];
    int res;

    for(;;) {
        if (error_flag == 0) {
            res = babelSockReadAll(client_sock, (char*)recvbuf, SERVER_PACKET_SIZE);
            if (res != SERVER_PACKET_SIZE) {
                printf("Client read error: %d\n", res);
                error_flag = 1;
                continue;
            }

            for (int i = 0; i < SERVER_MAX_NR_OF_PACKETS; i++) {
                const uint8_t *slot = recvbuf + CLIENT_PACKET_SIZE * i;
                int pt_len = crypto_open(plaintext,
                                         slot, CLIENT_PACKET_SIZE,
                                         g_remote_public_key,
                                         g_my_secret_key);
                if (pt_len != CLIENT_PLAIN_SIZE) {
                    continue;
                }
                /* Echo of our own outgoing packet — drop it.
                 * crypto_box's shared secret is symmetric in the keypair,
                 * so our own slots decrypt successfully too.
                 */
                if (memcmp(plaintext, g_my_public_key, CLIENT_KEY_SIZE) == 0) {
                    continue;
                }
                /* Real message from the configured remote peer. */
                uint8_t *text = plaintext + CLIENT_KEY_SIZE;
                text[CLIENT_TEXT_SIZE - 1] = '\0';
                printf("\n         ---(%s)---\n>>", (char*)text);
                fflush(stdout);
            }
        }
        babelThreadSleep(2000);
    }
    return NULL;
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
    char user_input[CLIENT_TEXT_SIZE];

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

    print_welcome();

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

    babelThreadInit();
    send_thread = babelThreadCreate(send_thread_func,
                                    &data,
                                    BABELTHREAD_PRIOHINT_LOW);
    babelThreadResume(send_thread);
    receive_thread = babelThreadCreate(receive_thread_func,
                                       &data,
                                       BABELTHREAD_PRIOHINT_LOW);
    babelThreadResume(receive_thread);
    printf("-> Heartbeat up and running!\n");
    printf("==========================================\n");

    while(1) {
        printf(">>");
        fflush(stdout);
        if (fgets(user_input, sizeof(user_input), stdin) == NULL) {
            break;
        }

        if (user_input[0] != '\n') {
            if (strcmp(user_input, "&genkeys\n") == 0) {
                gen_keys();
            } else {
                while (send_buf_flag != 0) {
                    babelThreadSleep(50);
                }
                memset(g_text_buffer, 0, CLIENT_TEXT_SIZE);
                strncpy(g_text_buffer, user_input, CLIENT_TEXT_SIZE - 1);
                send_buf_flag = 1;
            }
        }
    }
    babelSockClose(client_sock);
    babelSockCleanup();
    return 0;
}
