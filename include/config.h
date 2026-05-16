
#ifndef _CONFIG_H_
#define _CONFIG_H_

/* Server's compile-time defaults. The server advertises the negotiable
 * values (heartbeat, packet size, max clients) to clients via the
 * connect-time handshake (see include/protocol.h), so the client never
 * needs to know these at compile time. Crypto-related sizes live in
 * include/crypto.h so this header has no third-party dependencies.
 */

#define CLIENT_PACKET_SIZE   128

#define SERVER_PORT 27015
#define SERVER_HEART_BEAT_S 2
#define SERVER_MAX_NR_OF_CLIENTS 16
#define SERVER_PACKET_SIZE (SERVER_MAX_NR_OF_CLIENTS * CLIENT_PACKET_SIZE)
#define SERVER_MAX_NR_OF_PACKETS 16

#endif /* _CONFIG_H_ */
