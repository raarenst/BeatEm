
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "config.h"
#include "crypto.h"
#include "protocol.h"
#include "ws.h"

static int create_server(void);
static int send_buffer(void);
static int send_handshake(int sock);
static void remove_client(int sock);

static char g_recvbuf[CLIENT_PACKET_SIZE];
static int  g_client_list[SERVER_MAX_NR_OF_CLIENTS];
/* Per-client packet count for the current heartbeat. Cap = 1; any
 * additional packet from the same client within the same flush window
 * is silently dropped, preventing misbehaving senders from leaking
 * timing via early flushes.
 */
static int  g_client_packets[SERVER_MAX_NR_OF_CLIENTS];
static int  g_nr_clients;
static char g_sendbuf[SERVER_PACKET_SIZE];
static int  g_nr_packets;

int main(void) {
  int rv;
  int res;
  int added;
  int server_sock;
  int new_sock;
  time_t start_time_s;

  g_nr_packets = 0;

  if (crypto_init() != 0) {
    printf("*** ERROR: Could not initialize crypto library.\n");
    return -1;
  }

  server_sock = create_server();
  if (server_sock < 0) {
    printf("*** ERROR: Could not create server socket: %d\n", server_sock);
    return -1;
  }

  g_nr_clients = 0;
  for (int idx = 0; idx < SERVER_MAX_NR_OF_CLIENTS; idx++) {
    g_client_list[idx] = 0;
    g_client_packets[idx] = 0;
  }

  start_time_s = time(NULL);

  while (1) {
    /* Build the readable-fd set. Walk the full client array so a hole
     * left by a removed-but-not-compacted slot never becomes fd=0 in
     * the set.
     */
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(server_sock, &rfds);
    int max_fd = server_sock;
    for (int idx = 0; idx < SERVER_MAX_NR_OF_CLIENTS; idx++) {
      int s = g_client_list[idx];
      if (s != 0) {
        FD_SET(s, &rfds);
        if (s > max_fd) max_fd = s;
      }
    }

    struct timeval tv = {
      .tv_sec  = SERVER_HEART_BEAT_S,
      .tv_usec = 0,
    };
    res = select(max_fd + 1, &rfds, NULL, NULL, &tv);

    /* Time to flush? `>=` not `>`: with seconds-precision time, `>`
     * would stretch the cadence by one second and break the
     * constant-rate privacy property.
     */
    if (time(NULL) >= (start_time_s + SERVER_HEART_BEAT_S)) {
      send_buffer();
      g_nr_packets = 0;
      for (int idy = 0; idy < SERVER_MAX_NR_OF_CLIENTS; idy++) {
        g_client_packets[idy] = 0;
      }
      start_time_s = time(NULL);
    }

    if (res < 0) {
      if (errno == EINTR) continue;
      printf("*** ERROR: select: %s\n", strerror(errno));
      return 1;
    }
    if (res == 0) {
      /* timeout, nothing ready */
      continue;
    }

    /* New client? */
    if (FD_ISSET(server_sock, &rfds)) {
      new_sock = accept(server_sock, NULL, NULL);
      if (new_sock < 0) {
        printf("Could not accept client: %s\n", strerror(errno));
      } else if (ws_server_handshake(new_sock) != 0) {
        printf("WebSocket handshake failed; closing client.\n");
        close(new_sock);
      } else {
        added = 0;
        for (int idy = 0; idy < SERVER_MAX_NR_OF_CLIENTS; idy++) {
          if (g_client_list[idy] == 0) {
            g_client_list[idy] = new_sock;
            g_client_packets[idy] = 0;
            g_nr_clients++;
            added = 1;
            break;
          }
        }
        if (!added) {
          printf("Max nr of clients reached, refusing connection.\n");
          close(new_sock);
        } else if (send_handshake(new_sock) != 0) {
          printf("Protocol handshake send failed; closing client.\n");
          remove_client(new_sock);
          close(new_sock);
        }
      }
    }

    /* Existing clients with data? */
    for (int idx = 0; idx < SERVER_MAX_NR_OF_CLIENTS; idx++) {
      int s = g_client_list[idx];
      if (s == 0 || !FD_ISSET(s, &rfds)) continue;

      rv = ws_recv_binary(s, (uint8_t*)g_recvbuf, CLIENT_PACKET_SIZE);
      if (rv != CLIENT_PACKET_SIZE) {
        printf("*** ERROR: Receive from client error: %d\n", rv);
        close(s);
        remove_client(s);
      } else if (g_client_packets[idx] < 1) {
        g_client_packets[idx]++;
        memcpy(g_sendbuf + g_nr_packets * CLIENT_PACKET_SIZE,
               g_recvbuf, CLIENT_PACKET_SIZE);
        g_nr_packets++;
      }
      /* else: client is spamming within this heartbeat — drop silently. */
    }
  }

  close(server_sock);
  return 0;
}

static void remove_client(int sock) {
  for (int idy = 0; idy < SERVER_MAX_NR_OF_CLIENTS; idy++) {
    if (g_client_list[idy] == sock) {
      g_client_list[idy] = 0;
      g_client_packets[idy] = 0;
      g_nr_clients--;
      break;
    }
  }
}

static int send_buffer(void) {
  int startbuf;
  int res;

  /* Fill remaining buffer with CSPRNG bytes so unused slots are
   * statistically indistinguishable from real crypto_box ciphertext.
   */
  startbuf = g_nr_packets * CLIENT_PACKET_SIZE;
  crypto_random_bytes((uint8_t*)g_sendbuf + startbuf,
                      SERVER_PACKET_SIZE - startbuf);

  /* Send the whole buffer to every connected client as one WS binary
   * frame. Walk the full array so a hole left by an earlier removal
   * doesn't confuse the iteration.
   */
  for (int idx = 0; idx < SERVER_MAX_NR_OF_CLIENTS; idx++) {
    if (g_client_list[idx] == 0) continue;
    res = ws_send_binary(g_client_list[idx],
                         (const uint8_t*)g_sendbuf, SERVER_PACKET_SIZE, 0);
    if (res < 0) {
      printf("Client write error: %d\n", res);
      close(g_client_list[idx]);
      g_client_list[idx] = 0;
      g_client_packets[idx] = 0;
      g_nr_clients--;
    }
  }
  return 0;
}

static int send_handshake(int sock) {
  uint8_t buf[PROTO_HANDSHAKE_SIZE];
  proto_handshake_t h = {
    .version            = PROTO_VERSION,
    .max_clients        = SERVER_MAX_NR_OF_CLIENTS,
    .heartbeat_ms       = SERVER_HEART_BEAT_S * 1000,
    .client_packet_size = CLIENT_PACKET_SIZE,
  };
  proto_handshake_encode(buf, &h);
  int n = ws_send_binary(sock, buf, PROTO_HANDSHAKE_SIZE, 0);
  return (n > 0) ? 0 : -1;
}

static int create_server(void) {
  int server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock < 0) {
    printf("*** ERROR: socket: %s\n", strerror(errno));
    return -1;
  }
  int on = 1;
  if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
    printf("*** ERROR: setsockopt(SO_REUSEADDR): %s\n", strerror(errno));
    close(server_sock);
    return -1;
  }
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family      = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port        = htons(SERVER_PORT);
  if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    printf("*** ERROR: bind: %s\n", strerror(errno));
    close(server_sock);
    return -1;
  }
  if (listen(server_sock, 5) < 0) {
    printf("*** ERROR: listen: %s\n", strerror(errno));
    close(server_sock);
    return -1;
  }
  return server_sock;
}
