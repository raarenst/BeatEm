
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "config.h"
#include "crypto.h"
#include "protocol.h"
#include "ws.h"

/* Generated from files in web/ by the makefile's xxd rules. Each
 * header defines a `static unsigned char <name>[]` array; we use
 * sizeof() at the call site instead of the matching <name>_len
 * (which sed strips, since it would also trigger an unused-variable
 * warning). The server serves these on the same TCP port as the
 * WebSocket — no separate static file server needed.
 */
#include "index.html.h"
#include "manifest.json.h"
#include "sw.js.h"
#include "icon-192.png.h"
#include "icon-512.png.h"
#include "apple-touch-icon.png.h"

/* sizeof(array) is a compile-time constant; the xxd-generated *_len
 * symbols aren't, so use sizeof here to keep this static-initializable.
 */
#define ASSET(path, type, name) { (path), (type), (name), sizeof(name) }

static const ws_static_t g_assets[] = {
    ASSET("/",                     "text/html; charset=utf-8",  index_html),
    ASSET("/index.html",           "text/html; charset=utf-8",  index_html),
    ASSET("/manifest.json",        "application/manifest+json", manifest_json),
    ASSET("/sw.js",                "text/javascript",           sw_js),
    ASSET("/icon-192.png",         "image/png",                 icon_192_png),
    ASSET("/icon-512.png",         "image/png",                 icon_512_png),
    ASSET("/apple-touch-icon.png", "image/png",                 apple_touch_icon_png),
};
static const size_t g_n_assets = sizeof(g_assets) / sizeof(g_assets[0]);

/* Validation bounds — same as the client's HS_* checks, plus we
 * require packet_size to be large enough to hold one nonce + MAC +
 * sender_pk + replay counter + at least one byte of text.
 */
#define MIN_HEARTBEAT_MS  100u
#define MAX_HEARTBEAT_MS  60000u
#define MIN_MAX_CLIENTS   1u
#define MAX_MAX_CLIENTS   1024u
#define MIN_PACKET_SIZE   (CLIENT_NONCE_SIZE + CLIENT_MAC_SIZE + CLIENT_KEY_SIZE + CLIENT_COUNTER_SIZE + 1u)
#define MAX_PACKET_SIZE   65535u  /* WS u16 extended-length cap */

/* Runtime-configurable parameters. */
static uint32_t g_heartbeat_ms       = DEFAULT_HEARTBEAT_MS;
static uint32_t g_client_packet_size = DEFAULT_CLIENT_PACKET_SIZE;
static uint32_t g_max_clients        = DEFAULT_MAX_CLIENTS;
static uint16_t g_port               = DEFAULT_SERVER_PORT;
static size_t   g_server_packet_size = 0;

/* Heap-allocated once parameters are known. */
static uint8_t *g_recvbuf        = NULL;
static uint8_t *g_sendbuf        = NULL;
static int     *g_client_list    = NULL;
static int     *g_client_packets = NULL;
static int     *g_select_list    = NULL;

static int  g_nr_clients = 0;
static int  g_nr_packets = 0;

static int  create_server(uint16_t port);
static int  send_buffer(void);
static int  send_handshake(int sock);
static void remove_client(int sock);
static int  parse_args(int argc, char **argv);
static int  allocate_buffers(void);
static void usage(const char *prog);

int main(int argc, char **argv) {
  int rv, res, added;
  int server_sock, new_sock;
  time_t start_time_s;

  if (crypto_init() != 0) {
    printf("*** ERROR: Could not initialize crypto library.\n");
    return -1;
  }

  int rc = parse_args(argc, argv);
  if (rc != 0) return rc < 0 ? -1 : 0;  /* -1 = bad args, 1 = --help */

  if (allocate_buffers() != 0) {
    printf("*** ERROR: Could not allocate server buffers.\n");
    return -1;
  }

  server_sock = create_server(g_port);
  if (server_sock < 0) return -1;

  printf("BeatEm server listening on port %u "
         "(heartbeat=%u ms, packet=%u B, max_clients=%u)\n",
         g_port, g_heartbeat_ms, g_client_packet_size, g_max_clients);

  /* Convert ms -> seconds for the wall-clock comparison, rounded up so
   * a sub-second heartbeat still flushes at least once per second.
   * (The flush check uses time(NULL) which has 1-second resolution; a
   * runtime heartbeat below 1000 ms is honored to the nearest second.)
   */
  uint32_t flush_period_s = g_heartbeat_ms / 1000u;
  if (flush_period_s == 0) flush_period_s = 1;

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
    int select_count = 0;
    g_select_list[select_count++] = server_sock;
    for (uint32_t idx = 0; idx < g_max_clients; idx++) {
      int s = g_client_list[idx];
      if (s != 0) {
        FD_SET(s, &rfds);
        if (s > max_fd) max_fd = s;
        g_select_list[select_count++] = s;
      }
    }

    struct timeval tv = {
      .tv_sec  = (time_t)flush_period_s,
      .tv_usec = 0,
    };
    res = select(max_fd + 1, &rfds, NULL, NULL, &tv);

    /* Flush check.  `>=` so the actual cadence matches the advertised one. */
    if (time(NULL) >= (start_time_s + (time_t)flush_period_s)) {
      send_buffer();
      g_nr_packets = 0;
      for (uint32_t idy = 0; idy < g_max_clients; idy++) {
        g_client_packets[idy] = 0;
      }
      start_time_s = time(NULL);
    }

    if (res < 0) {
      if (errno == EINTR) continue;
      printf("*** ERROR: select: %s\n", strerror(errno));
      return 1;
    }
    if (res == 0) continue;

    if (FD_ISSET(server_sock, &rfds)) {
      new_sock = accept(server_sock, NULL, NULL);
      if (new_sock < 0) {
        printf("Could not accept client: %s\n", strerror(errno));
      } else {
        int disp = ws_serve_or_upgrade(new_sock, g_assets, g_n_assets);
        if (disp == WS_HTTP_DONE) {
          /* Served the static page (or a 404). Nothing else to do. */
          close(new_sock);
        } else if (disp != WS_UPGRADED) {
          /* Read error / malformed request / bad handshake. */
          close(new_sock);
        } else {
          /* WS upgrade succeeded. Find a slot and send the protocol
           * handshake; otherwise refuse for capacity. */
          added = 0;
          for (uint32_t idy = 0; idy < g_max_clients; idy++) {
            if (g_client_list[idy] == 0) {
              g_client_list[idy] = new_sock;
              g_client_packets[idy] = 0;
              g_nr_clients++;
              added = 1;
              break;
            }
          }
          if (!added) {
            printf("Max nr of clients (%u) reached, refusing connection.\n",
                   g_max_clients);
            close(new_sock);
          } else if (send_handshake(new_sock) != 0) {
            printf("Protocol handshake send failed; closing client.\n");
            remove_client(new_sock);
            close(new_sock);
          }
        }
      }
    }

    for (uint32_t idx = 0; idx < g_max_clients; idx++) {
      int s = g_client_list[idx];
      if (s == 0 || !FD_ISSET(s, &rfds)) continue;

      rv = ws_recv_binary(s, g_recvbuf, g_client_packet_size);
      if ((uint32_t)rv != g_client_packet_size) {
        printf("*** ERROR: Receive from client error: %d\n", rv);
        close(s);
        remove_client(s);
      } else if (g_client_packets[idx] < 1) {
        g_client_packets[idx]++;
        memcpy(g_sendbuf + (size_t)g_nr_packets * g_client_packet_size,
               g_recvbuf, g_client_packet_size);
        g_nr_packets++;
      }
      /* else: client is spamming within this heartbeat — drop silently. */
    }
  }

  close(server_sock);
  return 0;
}

static void remove_client(int sock) {
  for (uint32_t idy = 0; idy < g_max_clients; idy++) {
    if (g_client_list[idy] == sock) {
      g_client_list[idy] = 0;
      g_client_packets[idy] = 0;
      g_nr_clients--;
      break;
    }
  }
}

static int send_buffer(void) {
  size_t startbuf = (size_t)g_nr_packets * g_client_packet_size;
  crypto_random_bytes(g_sendbuf + startbuf, g_server_packet_size - startbuf);

  for (uint32_t idx = 0; idx < g_max_clients; idx++) {
    if (g_client_list[idx] == 0) continue;
    int r = ws_send_binary(g_client_list[idx],
                           g_sendbuf, g_server_packet_size, 0);
    if (r < 0) {
      printf("Client write error: %d\n", r);
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
    .max_clients        = (uint16_t)g_max_clients,
    .heartbeat_ms       = g_heartbeat_ms,
    .client_packet_size = g_client_packet_size,
  };
  proto_handshake_encode(buf, &h);
  int n = ws_send_binary(sock, buf, PROTO_HANDSHAKE_SIZE, 0);
  return (n > 0) ? 0 : -1;
}

static int create_server(uint16_t port) {
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
  addr.sin_port        = htons(port);
  if (bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    printf("*** ERROR: bind(%u): %s\n", port, strerror(errno));
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

static void usage(const char *prog) {
  printf(
    "Usage: %s [options]\n"
    "\n"
    "  --heartbeat-ms <ms>   flush cadence in milliseconds (default %u, range %u..%u)\n"
    "  --packet-size  <B>    bytes per client slot on the wire (default %u, range %u..%u)\n"
    "  --max-clients  <N>    maximum simultaneous clients (default %u, range %u..%u)\n"
    "  --port         <p>    TCP port (default %u)\n"
    "  --help                show this message and exit\n"
    "\n"
    "Note: the heartbeat is internally rounded down to whole seconds for the\n"
    "wall-clock flush check (so 1500 -> 1 s, 2500 -> 2 s). Sub-second is honored\n"
    "as 1 s minimum.\n",
    prog,
    DEFAULT_HEARTBEAT_MS,       MIN_HEARTBEAT_MS, MAX_HEARTBEAT_MS,
    DEFAULT_CLIENT_PACKET_SIZE, MIN_PACKET_SIZE,  MAX_PACKET_SIZE,
    DEFAULT_MAX_CLIENTS,        MIN_MAX_CLIENTS,  MAX_MAX_CLIENTS,
    DEFAULT_SERVER_PORT);
}

static int parse_args(int argc, char **argv) {
  static struct option longopts[] = {
    {"heartbeat-ms", required_argument, NULL, 'b'},
    {"packet-size",  required_argument, NULL, 'p'},
    {"max-clients",  required_argument, NULL, 'c'},
    {"port",         required_argument, NULL, 'P'},
    {"help",         no_argument,       NULL, 'h'},
    {0, 0, 0, 0}
  };
  int opt;
  while ((opt = getopt_long(argc, argv, "b:p:c:P:h", longopts, NULL)) != -1) {
    switch (opt) {
      case 'b': g_heartbeat_ms       = (uint32_t)strtoul(optarg, NULL, 10); break;
      case 'p': g_client_packet_size = (uint32_t)strtoul(optarg, NULL, 10); break;
      case 'c': g_max_clients        = (uint32_t)strtoul(optarg, NULL, 10); break;
      case 'P': g_port               = (uint16_t)strtoul(optarg, NULL, 10); break;
      case 'h': usage(argv[0]); return 1;
      default:  usage(argv[0]); return -1;
    }
  }

  if (g_heartbeat_ms < MIN_HEARTBEAT_MS || g_heartbeat_ms > MAX_HEARTBEAT_MS) {
    printf("--heartbeat-ms out of range (%u..%u)\n", MIN_HEARTBEAT_MS, MAX_HEARTBEAT_MS);
    return -1;
  }
  if (g_client_packet_size < MIN_PACKET_SIZE || g_client_packet_size > MAX_PACKET_SIZE) {
    printf("--packet-size out of range (%u..%u)\n", MIN_PACKET_SIZE, MAX_PACKET_SIZE);
    return -1;
  }
  if (g_max_clients < MIN_MAX_CLIENTS || g_max_clients > MAX_MAX_CLIENTS) {
    printf("--max-clients out of range (%u..%u)\n", MIN_MAX_CLIENTS, MAX_MAX_CLIENTS);
    return -1;
  }
  if (g_port == 0) {
    printf("--port must be non-zero\n");
    return -1;
  }

  g_server_packet_size = (size_t)g_client_packet_size * g_max_clients;
  return 0;
}

static int allocate_buffers(void) {
  g_recvbuf        = calloc(1, g_client_packet_size);
  g_sendbuf        = calloc(1, g_server_packet_size);
  g_client_list    = calloc(g_max_clients, sizeof(int));
  g_client_packets = calloc(g_max_clients, sizeof(int));
  g_select_list    = calloc(g_max_clients + 1, sizeof(int));
  if (!g_recvbuf || !g_sendbuf || !g_client_list ||
      !g_client_packets || !g_select_list) {
    return -1;
  }
  return 0;
}
