
#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <sodium.h>

/* Wire format
 * -----------
 * Every client packet is exactly CLIENT_PACKET_SIZE bytes on the wire:
 *
 *   [ 24-byte nonce ][ 16-byte Poly1305 MAC ][ 88-byte ciphertext ]
 *   <-- public ----> <----- crypto_box output (104B) ------------>
 *
 * The 88-byte ciphertext, once decrypted, contains:
 *
 *   [ 32-byte sender_pk ][ 56-byte text ]
 *
 * The embedded sender_pk lets a recipient distinguish "real packet from
 * peer" from "echo of my own packet". crypto_box's shared secret is
 * symmetric in the keypair (X25519(A_sk, B_pk) == X25519(B_sk, A_pk)),
 * so without an explicit sender identity inside the plaintext a client
 * would successfully decrypt its own outgoing slots when the server
 * broadcasts them back.
 *
 * Random (cover) packets fail authentication and are silently discarded.
 */

/* Client settings
 */
#define CLIENT_PACKET_SIZE   128
#define CLIENT_SEND_DELAY    2000
#define CLIENT_NONCE_SIZE    crypto_box_NONCEBYTES                 /* 24 */
#define CLIENT_MAC_SIZE      crypto_box_MACBYTES                   /* 16 */
#define CLIENT_KEY_SIZE      crypto_box_PUBLICKEYBYTES             /* 32 */
#define CLIENT_SECRET_SIZE   crypto_box_SECRETKEYBYTES             /* 32 */
#define CLIENT_PLAIN_SIZE    (CLIENT_PACKET_SIZE - CLIENT_NONCE_SIZE - CLIENT_MAC_SIZE) /* 88 */
#define CLIENT_TEXT_SIZE     (CLIENT_PLAIN_SIZE - CLIENT_KEY_SIZE) /* 56 */

/* Server settings
 */
#define SERVER_PORT 27015
#define SERVER_HEART_BEAT_S 2
#define SERVER_MAX_NR_OF_CLIENTS 16
#define SERVER_PACKET_SIZE (SERVER_MAX_NR_OF_CLIENTS*CLIENT_PACKET_SIZE)
#define SERVER_MAX_NR_OF_PACKETS 16

#endif /* _CONFIG_H_ */
