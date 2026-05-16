
#include <string.h>
#include <sodium.h>

#include "config.h"
#include "crypto.h"

int crypto_init(void) {
  return sodium_init() < 0 ? -1 : 0;
}

void crypto_keygen(uint8_t *pk, uint8_t *sk) {
  crypto_box_keypair(pk, sk);
}

int crypto_seal(uint8_t *out,
                const uint8_t *plain, size_t plain_len,
                const uint8_t *recipient_pk,
                const uint8_t *sender_sk) {
  uint8_t *nonce = out;
  uint8_t *ct    = out + crypto_box_NONCEBYTES;

  randombytes_buf(nonce, crypto_box_NONCEBYTES);
  if (crypto_box_easy(ct, plain, plain_len, nonce, recipient_pk, sender_sk) != 0) {
    return -1;
  }
  return (int)(crypto_box_NONCEBYTES + crypto_box_MACBYTES + plain_len);
}

int crypto_open(uint8_t *out,
                const uint8_t *packet, size_t packet_len,
                const uint8_t *sender_pk,
                const uint8_t *recipient_sk) {
  if (packet_len < crypto_box_NONCEBYTES + crypto_box_MACBYTES) {
    return -1;
  }
  const uint8_t *nonce = packet;
  const uint8_t *ct    = packet + crypto_box_NONCEBYTES;
  size_t ct_len        = packet_len - crypto_box_NONCEBYTES;

  if (crypto_box_open_easy(out, ct, ct_len, nonce, sender_pk, recipient_sk) != 0) {
    return -1;
  }
  return (int)(ct_len - crypto_box_MACBYTES);
}

void crypto_key_to_hex(char *hex, const uint8_t *key, size_t key_len) {
  sodium_bin2hex(hex, key_len * 2 + 1, key, key_len);
}

int crypto_hex_to_key(uint8_t *key, size_t key_len, const char *hex) {
  size_t bin_len = 0;
  if (sodium_hex2bin(key, key_len, hex, strlen(hex),
                     NULL, &bin_len, NULL) != 0) {
    return -1;
  }
  if (bin_len != key_len) {
    return -1;
  }
  return 0;
}
