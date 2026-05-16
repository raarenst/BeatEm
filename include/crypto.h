
#ifndef _CRYPTO_H_
#define _CRYPTO_H_

#include <stdint.h>
#include <stddef.h>

/* Initialize the underlying crypto library. Call once at program start.
 * Returns 0 on success, -1 on failure.
 */
int crypto_init(void);

/* Generate a fresh X25519 keypair.
 *   pk: 32 bytes (CLIENT_KEY_SIZE)
 *   sk: 32 bytes (CLIENT_SECRET_SIZE)
 */
void crypto_keygen(uint8_t *pk, uint8_t *sk);

/* Seal a plaintext message into an authenticated, encrypted packet.
 *
 * Packet layout written to `out`:
 *   [ 24-byte nonce ][ plain_len + 16 ciphertext+MAC ]
 *
 * `out` must have room for 24 + 16 + plain_len bytes.
 * Returns total bytes written, or -1 on failure.
 */
int crypto_seal(uint8_t *out,
                const uint8_t *plain, size_t plain_len,
                const uint8_t *recipient_pk,
                const uint8_t *sender_sk);

/* Try to open a packet. Returns plaintext length on success, -1 on auth
 * failure (wrong recipient, wrong sender, or tampered/random padding).
 *
 * `packet_len` must be at least 24 + 16. `out` must have room for
 * `packet_len - 24 - 16` bytes.
 */
int crypto_open(uint8_t *out,
                const uint8_t *packet, size_t packet_len,
                const uint8_t *sender_pk,
                const uint8_t *recipient_sk);

/* Hex helpers. The hex buffer must be at least 2*key_len+1 bytes. */
void crypto_key_to_hex(char *hex, const uint8_t *key, size_t key_len);
int  crypto_hex_to_key(uint8_t *key, size_t key_len, const char *hex);

#endif /* _CRYPTO_H_ */
