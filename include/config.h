
#ifndef _CONFIG_H_
#define _CONFIG_H_

/* Server defaults. All of these are overridable at runtime via CLI
 * flags (`--heartbeat-ms`, `--packet-size`, `--max-clients`, `--port`)
 * and the chosen values are advertised to each client through the
 * connect-time handshake (see include/protocol.h). Clients never read
 * these at compile time — they derive every size from the handshake.
 *
 * Crypto-related sizes live in include/crypto.h so this header has no
 * third-party dependencies.
 */

#define DEFAULT_CLIENT_PACKET_SIZE   128u   /* bytes per slot on the wire */
#define DEFAULT_HEARTBEAT_MS         2000u  /* flush cadence              */
#define DEFAULT_MAX_CLIENTS          16u    /* maximum simultaneous clients */
#define DEFAULT_SERVER_PORT          27015u

#endif /* _CONFIG_H_ */
