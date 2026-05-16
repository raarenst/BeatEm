
#include <stdlib.h>
#include <stdio.h>
#include "config.h"
#include "crypto.h"
#include "protocol.h"
#include "babelsock.h"
#include "babeltime.h"

static int create_server();
static int send_buffer();
static int send_handshake(int sock);
static void remove_client(int sock);

static char g_recvbuf[CLIENT_PACKET_SIZE];
static int g_client_list[SERVER_MAX_NR_OF_CLIENTS];
static int g_nr_clients;
static char g_sendbuf[SERVER_PACKET_SIZE];
static int g_nr_packets;

int main(void) {
  int rv;
  int res;
  int added;
  int server_sock;
  int new_sock;
  int select_list[SERVER_MAX_NR_OF_CLIENTS + 1]; /* + 1 for server socket */
  int select_count;
  long start_time_s;

  g_nr_packets = 0;

  if (crypto_init() != 0) {
    printf("*** ERROR: Could not initialize crypto library.\n");
    return -1;
  }

  /* Setup the server
   */
  server_sock = create_server();
  if (server_sock < 0) {
    printf("*** ERROR: Could not create server socket: %d\n", server_sock);
    return -1;
  }

  /* Setup client list
   */
  g_nr_clients = 0;
  for (int idx = 0; idx < SERVER_MAX_NR_OF_CLIENTS; idx++) {
    g_client_list[idx] = 0;
  }

  start_time_s = babelTimeGetCurrentTime();

  /* Main loop
   */
  while (1) {

    /* Build the select list. Walk the full client array and skip zero
     * slots so a removed-but-not-compacted slot never enters select()
     * as fd=0 (== stdin).
     */
    select_count = 0;
    select_list[select_count++] = server_sock;
    for (int idx = 0; idx < SERVER_MAX_NR_OF_CLIENTS; idx++) {
      if (g_client_list[idx] != 0) {
        select_list[select_count++] = g_client_list[idx];
      }
    }

    /* Wait for server, any client or timeout
     */
    res = babelSockSelect(select_list, select_count, SERVER_HEART_BEAT_S * 1000);

    /* Time to send, send the buffer before receiving more.
     * `>=` not `>`: with seconds-precision time, `>` would require
     * `now >= start + heartbeat + 1` and stretch the actual cadence
     * to heartbeat+1 seconds — which would mismatch what the server
     * advertises to clients in the handshake and weaken the
     * constant-cadence privacy property.
     */
    if (babelTimeGetCurrentTime() >= (start_time_s + SERVER_HEART_BEAT_S)) {
      send_buffer();
      g_nr_packets = 0;
      start_time_s = babelTimeGetCurrentTime();
    }

    /* Handle the select results
     */
    if (res < 0) {
      printf("*** ERROR: Select: %d\n", res);
      return 1;
    } else if (res == 0) {
      /* Timeout, nothing to do
       */
    } else {

      /* It is a socket that wants something
       */
      for (int idx = 0; idx < res; idx++) {
        if (select_list[idx] == server_sock) {

          /* New client connecting. Find a slot first, *then* send the
           * handshake — avoids handshaking a client we're about to
           * refuse for capacity reasons.
           */
          new_sock = babelSockAccept(server_sock);
          if (new_sock < 0) {
            printf("Could not accept client: %d\n", new_sock);
          } else {
            added = 0;
            for (int idy = 0; idy < SERVER_MAX_NR_OF_CLIENTS; idy++) {
              if (g_client_list[idy] == 0) {
                g_client_list[idy] = new_sock;
                g_nr_clients++;
                added = 1;
                break;
              }
            }
            if (!added) {
              printf("Max nr of clients reached, refusing connection.\n");
              babelSockClose(new_sock);
            } else if (send_handshake(new_sock) != 0) {
              printf("Handshake send failed; closing client.\n");
              remove_client(new_sock);
              babelSockClose(new_sock);
            }
          }
        } else {

          /* It is a client sending a packet or disconnecting
           */
          rv = babelSockReadAll(select_list[idx], g_recvbuf, CLIENT_PACKET_SIZE);
          if (rv != CLIENT_PACKET_SIZE) {
            printf("*** ERROR: Receive from client error: %d\n", rv);
            babelSockClose(select_list[idx]);
            remove_client(select_list[idx]);
          } else {

            /* Add buf to send buffer
             */
            if (g_nr_packets == SERVER_MAX_NR_OF_PACKETS) {
              /* Buffer full — flush before adding the new packet.
               * FIXME: should rate-limit spammy clients.
               */
              send_buffer();
              g_nr_packets = 0;
            }
            for (int idy = 0; idy < CLIENT_PACKET_SIZE; idy++) {
              g_sendbuf[g_nr_packets * CLIENT_PACKET_SIZE + idy] = g_recvbuf[idy];
            }
            g_nr_packets++;
          }
        }
      }
    }
  }

  babelSockClose(server_sock);
  babelSockCleanup();

  return 0;
}

static void remove_client(int sock) {
  for (int idy = 0; idy < SERVER_MAX_NR_OF_CLIENTS; idy++) {
    if (g_client_list[idy] == sock) {
      g_client_list[idy] = 0;
      g_nr_clients--;
      break;
    }
  }
}

int send_buffer() {
  int startbuf;
  int res;

  /* Fill remaining buffer with cryptographic randomness so unused slots
   * are statistically indistinguishable from real crypto_box ciphertext.
   */
  startbuf = g_nr_packets * CLIENT_PACKET_SIZE;
  crypto_random_bytes((uint8_t*)g_sendbuf + startbuf,
                      SERVER_PACKET_SIZE - startbuf);

  /* Send whole buffer to every connected client. Walk the full array so
   * a removed-but-not-compacted slot can never confuse the iteration.
   */
  for (int idx = 0; idx < SERVER_MAX_NR_OF_CLIENTS; idx++) {
    if (g_client_list[idx] == 0) continue;
    res = babelSockWriteAll(g_client_list[idx], g_sendbuf, SERVER_PACKET_SIZE);
    if (res != SERVER_PACKET_SIZE) {
      printf("Client write error: %d\n", res);
      babelSockClose(g_client_list[idx]);
      g_client_list[idx] = 0;
      g_nr_clients--;
    }
  }
  return 0;
}

int send_handshake(int sock) {
  uint8_t buf[PROTO_HANDSHAKE_SIZE];
  proto_handshake_t h = {
    .version            = PROTO_VERSION,
    .max_clients        = SERVER_MAX_NR_OF_CLIENTS,
    .heartbeat_ms       = SERVER_HEART_BEAT_S * 1000,
    .client_packet_size = CLIENT_PACKET_SIZE,
  };
  proto_handshake_encode(buf, &h);
  int n = babelSockWriteAll(sock, (char*)buf, PROTO_HANDSHAKE_SIZE);
  return (n == PROTO_HANDSHAKE_SIZE) ? 0 : -1;
}

int create_server() {
  int res;
  int server_sock;

  res = babelSockInit();
  if (res != BABELSOCK_OK) {
    printf("*** ERROR: Could not initialize babelsock: %d\n", res);
    return -1;
  }

  server_sock = babelSock(BABELSOCK_TCP);
  if (server_sock < 0) {
    babelSockCleanup();
    printf("*** ERROR: Could not create server socket: %d\n", server_sock);
    return -1;
  }

  res = babelSockBind(server_sock, "0.0.0.0", SERVER_PORT);
  if (res != BABELSOCK_OK) {
    babelSockClose(server_sock);
    babelSockCleanup();
    printf("*** ERROR: Could not bind server socket: %d\n", res);
    return -1;
  }

  res = babelSockListen(server_sock, 5);
  if (res != BABELSOCK_OK) {
    babelSockClose(server_sock);
    babelSockCleanup();
    printf("*** ERROR: Could not set server socket to listen: %d\n", res);
    return -1;
  }

  return server_sock;
}
