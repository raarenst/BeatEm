
#ifndef _WS_H_
#define _WS_H_

#include <stdint.h>
#include <stddef.h>

/* Minimal RFC 6455 WebSocket layer for BeatEm.
 *
 * Scope: plain ws:// over a connected TCP socket. We carry binary
 * frames in both directions (no fragmentation, no extensions, no
 * permessage-deflate). The server never masks (RFC 6455 §5.1); the
 * client always masks.
 *
 * Frame sizes we actually use:
 *   - Server -> client broadcast: ~2 KB (uses 16-bit extended length)
 *   - Client -> server packet:    128 bytes (16-bit extended length;
 *                                  short form caps at 125)
 *   - Connect-time handshake:     16 bytes (short form, no extension)
 *
 * TLS (wss://) is out of scope here — run behind a reverse proxy
 * (nginx, caddy) if you need it.
 */

#define WS_MAX_PAYLOAD  65535u

/* Server side: consume the client's HTTP upgrade request from `sock`
 * and write back the 101 Switching Protocols response with the right
 * Sec-WebSocket-Accept. Returns 0 on success, -1 on protocol error or
 * I/O failure.
 */
int ws_server_handshake(int sock);

/* Client side: send a GET upgrade request for the given host/port and
 * verify the 101 response. Returns 0 on success, -1 on failure.
 * `host` is used only for the Host: header (the socket is already
 * connected).
 */
int ws_client_handshake(int sock, const char *host, int port);

/* Send `len` payload bytes as a single binary frame.
 *   is_client = 1 -> mask the payload (RFC requires client masking)
 *   is_client = 0 -> no mask
 * Returns total wire bytes written, or -1 on failure.
 */
int ws_send_binary(int sock, const uint8_t *payload, size_t len, int is_client);

/* Receive one binary frame; write payload into `out` (capacity
 * max_len). Returns payload length on success, or -1 on error/close.
 *
 * Transparently responds to ping with pong, silently drops text frames,
 * and treats a close frame as an error.
 */
int ws_recv_binary(int sock, uint8_t *out, size_t max_len);

#endif /* _WS_H_ */
