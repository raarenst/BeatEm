
#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include <stdint.h>

/* Connect-time handshake (server -> client, sent immediately on accept).
 *
 * Wire format: 16 bytes, all multi-byte fields big-endian.
 *
 *   offset  size  field
 *   ------  ----  -----
 *     0       4   magic           "BEAT" (0x42 0x45 0x41 0x54)
 *     4       2   version         current = 1
 *     6       2   max_clients     server's SERVER_MAX_NR_OF_CLIENTS
 *     8       4   heartbeat_ms    server's flush cadence (= client send cadence)
 *    12       4   client_packet_size   bytes per client slot in the broadcast
 *
 * After parsing, the client derives:
 *   server_packet_size = max_clients * client_packet_size
 *   plain_size         = client_packet_size - 24 (nonce) - 16 (MAC)
 *   text_size          = plain_size - 32 (sender_pk) - 4 (replay counter)
 */

#define PROTO_HANDSHAKE_SIZE   16
#define PROTO_MAGIC_0          'B'
#define PROTO_MAGIC_1          'E'
#define PROTO_MAGIC_2          'A'
#define PROTO_MAGIC_3          'T'
#define PROTO_VERSION          1

typedef struct {
    uint16_t version;
    uint16_t max_clients;
    uint32_t heartbeat_ms;
    uint32_t client_packet_size;
} proto_handshake_t;

/* Pack a handshake into 16 bytes. `buf` must have room for PROTO_HANDSHAKE_SIZE. */
void proto_handshake_encode(uint8_t *buf, const proto_handshake_t *h);

/* Parse 16 bytes into a handshake struct. Returns 0 on success, -1 on bad
 * magic or version mismatch. Caller is still responsible for validating
 * the field values (sane ranges) before allocating from them.
 */
int proto_handshake_decode(const uint8_t *buf, proto_handshake_t *h);

/* Replay-protection check. `plaintext` is the decrypted payload of an
 * authenticated packet (layout: [sender_pk (32)][counter (4 BE)][text]).
 * Parses the embedded counter and compares against *highest_seen.
 *
 *   returns 1 (and updates *highest_seen) if the counter is strictly
 *           greater than the previously seen value — i.e., the packet
 *           should be processed.
 *   returns 0 if the counter is <= *highest_seen — i.e., the packet
 *           is a replay or reorder and should be dropped.
 */
int proto_counter_check_and_update(const uint8_t *plaintext,
                                   uint32_t *highest_seen);

#endif /* _PROTOCOL_H_ */
