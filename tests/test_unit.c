/* Unit tests for crypto.c and protocol.c.
 *
 * Plain-C, no test framework — each TEST() prints PASS/FAIL and bumps
 * a counter. main() exits non-zero if any test failed so the makefile
 * can detect it.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "crypto.h"
#include "protocol.h"

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, msg) do {                                          \
    if (cond) {                                                        \
        g_pass++;                                                      \
        printf("  PASS: %s\n", msg);                                   \
    } else {                                                           \
        g_fail++;                                                      \
        printf("  FAIL: %s  (at %s:%d)\n", msg, __FILE__, __LINE__);   \
    }                                                                  \
} while (0)

static void test_hex_roundtrip(void) {
    printf("test_hex_roundtrip:\n");
    uint8_t key[CLIENT_KEY_SIZE];
    for (size_t i = 0; i < sizeof(key); i++) key[i] = (uint8_t)(i * 7 + 3);

    char hex[CLIENT_KEY_SIZE * 2 + 1];
    crypto_key_to_hex(hex, key, CLIENT_KEY_SIZE);
    CHECK(strlen(hex) == CLIENT_KEY_SIZE * 2, "hex length matches");

    uint8_t roundtrip[CLIENT_KEY_SIZE];
    int rc = crypto_hex_to_key(roundtrip, CLIENT_KEY_SIZE, hex);
    CHECK(rc == 0, "valid hex parses");
    CHECK(memcmp(key, roundtrip, CLIENT_KEY_SIZE) == 0, "hex round-trip preserves bytes");

    /* Bad hex: non-hex characters */
    uint8_t junk[CLIENT_KEY_SIZE];
    rc = crypto_hex_to_key(junk, CLIENT_KEY_SIZE, "zzznotvalidhex");
    CHECK(rc != 0, "non-hex string rejected");

    /* Too short */
    rc = crypto_hex_to_key(junk, CLIENT_KEY_SIZE, "deadbeef");
    CHECK(rc != 0, "short hex rejected");
}

static void test_crypto_seal_open_roundtrip(void) {
    printf("test_crypto_seal_open_roundtrip:\n");

    uint8_t alice_pk[CLIENT_KEY_SIZE], alice_sk[CLIENT_SECRET_SIZE];
    uint8_t bob_pk[CLIENT_KEY_SIZE],   bob_sk[CLIENT_SECRET_SIZE];
    crypto_keygen(alice_pk, alice_sk);
    crypto_keygen(bob_pk, bob_sk);

    const size_t plain_len = 88;  /* matches default packet plain size */
    uint8_t plain[88], opened[88], packet[128];
    for (size_t i = 0; i < plain_len; i++) plain[i] = (uint8_t)(i ^ 0xA5);

    int sealed = crypto_seal(packet, plain, plain_len, bob_pk, alice_sk);
    CHECK(sealed == 128, "seal returns 24 + 16 + plain_len bytes");

    int opened_len = crypto_open(opened, packet, sealed, alice_pk, bob_sk);
    CHECK(opened_len == (int)plain_len, "open returns plaintext length");
    CHECK(memcmp(plain, opened, plain_len) == 0, "round-trip preserves bytes");

    /* Open with wrong recipient secret key — must fail */
    uint8_t mallory_pk[CLIENT_KEY_SIZE], mallory_sk[CLIENT_SECRET_SIZE];
    crypto_keygen(mallory_pk, mallory_sk);
    opened_len = crypto_open(opened, packet, sealed, alice_pk, mallory_sk);
    CHECK(opened_len < 0, "open rejects wrong recipient key");

    /* Open with wrong sender public key — must fail */
    opened_len = crypto_open(opened, packet, sealed, mallory_pk, bob_sk);
    CHECK(opened_len < 0, "open rejects wrong sender key");

    /* Tampered ciphertext — flip a byte, must fail */
    packet[64] ^= 1;
    opened_len = crypto_open(opened, packet, sealed, alice_pk, bob_sk);
    CHECK(opened_len < 0, "open rejects tampered ciphertext");

    /* Random-bytes "cover" packet — must fail to auth */
    uint8_t random_packet[128];
    crypto_random_bytes(random_packet, sizeof(random_packet));
    opened_len = crypto_open(opened, random_packet, sizeof(random_packet),
                             alice_pk, bob_sk);
    CHECK(opened_len < 0, "open rejects random (cover) bytes");
}

static void test_handshake_roundtrip(void) {
    printf("test_handshake_roundtrip:\n");

    proto_handshake_t h = {
        .version            = PROTO_VERSION,
        .max_clients        = 16,
        .heartbeat_ms       = 2000,
        .client_packet_size = 128,
    };
    uint8_t buf[PROTO_HANDSHAKE_SIZE];
    proto_handshake_encode(buf, &h);

    CHECK(buf[0] == 'B' && buf[1] == 'E' && buf[2] == 'A' && buf[3] == 'T',
          "magic encoded as ASCII BEAT");

    proto_handshake_t out;
    int rc = proto_handshake_decode(buf, &out);
    CHECK(rc == 0, "valid handshake decodes");
    CHECK(out.version == h.version, "version round-trip");
    CHECK(out.max_clients == h.max_clients, "max_clients round-trip");
    CHECK(out.heartbeat_ms == h.heartbeat_ms, "heartbeat_ms round-trip");
    CHECK(out.client_packet_size == h.client_packet_size, "client_packet_size round-trip");

    /* Bad magic */
    buf[0] = 'X';
    rc = proto_handshake_decode(buf, &out);
    CHECK(rc != 0, "bad magic rejected");
    buf[0] = 'B';

    /* Bad version */
    buf[4] = 0xFF; buf[5] = 0xFF;
    rc = proto_handshake_decode(buf, &out);
    CHECK(rc != 0, "unsupported version rejected");
}

static void test_replay_counter(void) {
    printf("test_replay_counter:\n");

    /* Build a fake decrypted plaintext: 32B pk + 4B counter + ignored. */
    uint8_t pt[64] = {0};
    uint32_t highest = 0;

    /* First packet: counter=100. Should be accepted. */
    pt[CLIENT_KEY_SIZE + 0] = 0;
    pt[CLIENT_KEY_SIZE + 1] = 0;
    pt[CLIENT_KEY_SIZE + 2] = 0;
    pt[CLIENT_KEY_SIZE + 3] = 100;
    CHECK(proto_counter_check_and_update(pt, &highest) == 1,
          "first packet (counter=100) accepted");
    CHECK(highest == 100, "highest_seen updated to 100");

    /* Replay of the same counter. Must be dropped. */
    CHECK(proto_counter_check_and_update(pt, &highest) == 0,
          "replay (same counter=100) dropped");
    CHECK(highest == 100, "highest_seen unchanged after replay");

    /* Lower counter. Must be dropped (reorder protection). */
    pt[CLIENT_KEY_SIZE + 3] = 50;
    CHECK(proto_counter_check_and_update(pt, &highest) == 0,
          "out-of-order (counter=50 < 100) dropped");
    CHECK(highest == 100, "highest_seen unchanged after reorder");

    /* Higher counter. Must be accepted. */
    pt[CLIENT_KEY_SIZE + 2] = 1;     /* upper byte → 256 + 50 = 306 */
    pt[CLIENT_KEY_SIZE + 3] = 50;
    CHECK(proto_counter_check_and_update(pt, &highest) == 1,
          "higher counter (306 > 100) accepted");
    CHECK(highest == 306, "highest_seen updated to 306");

    /* Big-endian sanity: 0x12345678 must parse byte-correctly. */
    highest = 0;
    pt[CLIENT_KEY_SIZE + 0] = 0x12;
    pt[CLIENT_KEY_SIZE + 1] = 0x34;
    pt[CLIENT_KEY_SIZE + 2] = 0x56;
    pt[CLIENT_KEY_SIZE + 3] = 0x78;
    CHECK(proto_counter_check_and_update(pt, &highest) == 1,
          "big-endian 0x12345678 accepted");
    CHECK(highest == 0x12345678, "big-endian byte order parsed correctly");
}

int main(void) {
    if (crypto_init() != 0) {
        fprintf(stderr, "crypto_init failed\n");
        return 1;
    }

    test_hex_roundtrip();
    test_crypto_seal_open_roundtrip();
    test_handshake_roundtrip();
    test_replay_counter();

    printf("\nTotal: %d passed, %d failed\n", g_pass, g_fail);
    return g_fail == 0 ? 0 : 1;
}
