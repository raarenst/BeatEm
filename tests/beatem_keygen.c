/* Test helper: print two libsodium X25519 keypairs as KEYPAIR_N_{SK,PK}=hex
 * lines, one per line, ready to be read by smoke_test.sh.
 */
#include <stdio.h>
#include <sodium.h>

int main(void) {
    if (sodium_init() < 0) {
        fprintf(stderr, "sodium_init failed\n");
        return 1;
    }
    unsigned char pk[crypto_box_PUBLICKEYBYTES];
    unsigned char sk[crypto_box_SECRETKEYBYTES];
    char pk_hex[crypto_box_PUBLICKEYBYTES * 2 + 1];
    char sk_hex[crypto_box_SECRETKEYBYTES * 2 + 1];

    for (int i = 0; i < 2; i++) {
        crypto_box_keypair(pk, sk);
        sodium_bin2hex(pk_hex, sizeof(pk_hex), pk, sizeof(pk));
        sodium_bin2hex(sk_hex, sizeof(sk_hex), sk, sizeof(sk));
        printf("KEYPAIR_%d_SK=%s\n", i, sk_hex);
        printf("KEYPAIR_%d_PK=%s\n", i, pk_hex);
    }
    return 0;
}
