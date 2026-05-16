/* Minimal RFC 6455 WebSocket implementation for BeatEm.
 *
 * Vendor-quality enough for a single-purpose chat protocol, not a
 * general-purpose library. See include/ws.h for the public contract.
 *
 * Notes:
 *   - We always send a complete payload as a single un-fragmented frame
 *     (FIN=1). RFC 6455 allows servers/clients to fragment, but our
 *     protocol packets are tiny and there's no upside.
 *   - We don't support permessage-deflate or any extension.
 *   - SHA-1 is vendored below (public domain, Steve Reid). RFC 6455
 *     mandates SHA-1 for the Sec-WebSocket-Accept value; the hash is
 *     used as a non-cryptographic protocol marker, so the modern
 *     deprecation of SHA-1 for security is not a concern here.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>      /* strncasecmp */
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sodium.h>

#include "ws.h"

#define WS_GUID   "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_OP_CONT   0x0
#define WS_OP_TEXT   0x1
#define WS_OP_BINARY 0x2
#define WS_OP_CLOSE  0x8
#define WS_OP_PING   0x9
#define WS_OP_PONG   0xA

/* -------------------- Public-domain SHA-1 (Steve Reid) -------------------- */

typedef struct {
    uint32_t state[5];
    uint32_t count[2];   /* bit count, [high, low] */
    uint8_t  buffer[64];
} sha1_ctx;

#define ROL(v, b) (((v) << (b)) | ((v) >> (32 - (b))))

#define BLK0(i) (block->l[i] = (ROL(block->l[i], 24) & 0xFF00FF00) | (ROL(block->l[i], 8) & 0x00FF00FF))
#define BLK(i)  (block->l[i & 15] = ROL(block->l[(i + 13) & 15] ^ block->l[(i + 8) & 15] ^ block->l[(i + 2) & 15] ^ block->l[i & 15], 1))

#define R0(v,w,x,y,z,i) z += ((w & (x ^ y)) ^ y) + BLK0(i) + 0x5A827999 + ROL(v, 5); w = ROL(w, 30);
#define R1(v,w,x,y,z,i) z += ((w & (x ^ y)) ^ y) + BLK(i)  + 0x5A827999 + ROL(v, 5); w = ROL(w, 30);
#define R2(v,w,x,y,z,i) z += (w ^ x ^ y)         + BLK(i)  + 0x6ED9EBA1 + ROL(v, 5); w = ROL(w, 30);
#define R3(v,w,x,y,z,i) z += (((w | x) & y) | (w & x)) + BLK(i) + 0x8F1BBCDC + ROL(v, 5); w = ROL(w, 30);
#define R4(v,w,x,y,z,i) z += (w ^ x ^ y)         + BLK(i)  + 0xCA62C1D6 + ROL(v, 5); w = ROL(w, 30);

static void sha1_transform(uint32_t state[5], const uint8_t buf[64]) {
    typedef union {
        uint8_t  c[64];
        uint32_t l[16];
    } block_t;
    block_t block_storage;
    block_t *block = &block_storage;
    memcpy(block, buf, 64);

    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], e = state[4];

    R0(a,b,c,d,e, 0); R0(e,a,b,c,d, 1); R0(d,e,a,b,c, 2); R0(c,d,e,a,b, 3);
    R0(b,c,d,e,a, 4); R0(a,b,c,d,e, 5); R0(e,a,b,c,d, 6); R0(d,e,a,b,c, 7);
    R0(c,d,e,a,b, 8); R0(b,c,d,e,a, 9); R0(a,b,c,d,e,10); R0(e,a,b,c,d,11);
    R0(d,e,a,b,c,12); R0(c,d,e,a,b,13); R0(b,c,d,e,a,14); R0(a,b,c,d,e,15);
    R1(e,a,b,c,d,16); R1(d,e,a,b,c,17); R1(c,d,e,a,b,18); R1(b,c,d,e,a,19);
    R2(a,b,c,d,e,20); R2(e,a,b,c,d,21); R2(d,e,a,b,c,22); R2(c,d,e,a,b,23);
    R2(b,c,d,e,a,24); R2(a,b,c,d,e,25); R2(e,a,b,c,d,26); R2(d,e,a,b,c,27);
    R2(c,d,e,a,b,28); R2(b,c,d,e,a,29); R2(a,b,c,d,e,30); R2(e,a,b,c,d,31);
    R2(d,e,a,b,c,32); R2(c,d,e,a,b,33); R2(b,c,d,e,a,34); R2(a,b,c,d,e,35);
    R2(e,a,b,c,d,36); R2(d,e,a,b,c,37); R2(c,d,e,a,b,38); R2(b,c,d,e,a,39);
    R3(a,b,c,d,e,40); R3(e,a,b,c,d,41); R3(d,e,a,b,c,42); R3(c,d,e,a,b,43);
    R3(b,c,d,e,a,44); R3(a,b,c,d,e,45); R3(e,a,b,c,d,46); R3(d,e,a,b,c,47);
    R3(c,d,e,a,b,48); R3(b,c,d,e,a,49); R3(a,b,c,d,e,50); R3(e,a,b,c,d,51);
    R3(d,e,a,b,c,52); R3(c,d,e,a,b,53); R3(b,c,d,e,a,54); R3(a,b,c,d,e,55);
    R3(e,a,b,c,d,56); R3(d,e,a,b,c,57); R3(c,d,e,a,b,58); R3(b,c,d,e,a,59);
    R4(a,b,c,d,e,60); R4(e,a,b,c,d,61); R4(d,e,a,b,c,62); R4(c,d,e,a,b,63);
    R4(b,c,d,e,a,64); R4(a,b,c,d,e,65); R4(e,a,b,c,d,66); R4(d,e,a,b,c,67);
    R4(c,d,e,a,b,68); R4(b,c,d,e,a,69); R4(a,b,c,d,e,70); R4(e,a,b,c,d,71);
    R4(d,e,a,b,c,72); R4(c,d,e,a,b,73); R4(b,c,d,e,a,74); R4(a,b,c,d,e,75);
    R4(e,a,b,c,d,76); R4(d,e,a,b,c,77); R4(c,d,e,a,b,78); R4(b,c,d,e,a,79);

    state[0] += a; state[1] += b; state[2] += c; state[3] += d; state[4] += e;
}

static void sha1_init(sha1_ctx *ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count[0] = ctx->count[1] = 0;
}

static void sha1_update(sha1_ctx *ctx, const uint8_t *data, size_t len) {
    uint32_t i, j;
    j = (ctx->count[0] >> 3) & 63;
    if ((ctx->count[0] += (uint32_t)(len << 3)) < (uint32_t)(len << 3)) ctx->count[1]++;
    ctx->count[1] += (uint32_t)(len >> 29);
    if (j + len > 63) {
        i = 64 - j;
        memcpy(&ctx->buffer[j], data, i);
        sha1_transform(ctx->state, ctx->buffer);
        for (; i + 63 < len; i += 64) sha1_transform(ctx->state, &data[i]);
        j = 0;
    } else {
        i = 0;
    }
    memcpy(&ctx->buffer[j], &data[i], len - i);
}

static void sha1_final(uint8_t digest[20], sha1_ctx *ctx) {
    uint8_t finalcount[8];
    for (int i = 0; i < 8; i++) {
        finalcount[i] = (uint8_t)((ctx->count[(i >= 4 ? 0 : 1)] >> ((3 - (i & 3)) * 8)) & 0xFF);
    }
    uint8_t c = 0x80;
    sha1_update(ctx, &c, 1);
    while ((ctx->count[0] & 504) != 448) {
        c = 0x00;
        sha1_update(ctx, &c, 1);
    }
    sha1_update(ctx, finalcount, 8);
    for (int i = 0; i < 20; i++) {
        digest[i] = (uint8_t)((ctx->state[i >> 2] >> ((3 - (i & 3)) * 8)) & 0xFF);
    }
}

/* -------------------- Socket I/O helpers -------------------- */

static int read_exact(int sock, uint8_t *buf, size_t n) {
    size_t got = 0;
    while (got < n) {
        ssize_t r = recv(sock, buf + got, n - got, 0);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return -1;
        got += (size_t)r;
    }
    return 0;
}

static int write_exact(int sock, const uint8_t *buf, size_t n) {
    size_t wrote = 0;
    while (wrote < n) {
        ssize_t w = send(sock, buf + wrote, n - wrote, 0);
        if (w < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        wrote += (size_t)w;
    }
    return 0;
}

/* -------------------- HTTP upgrade -------------------- */

/* Find a case-insensitive header line "Name: value\r\n" in `headers`
 * and copy its value (up to value_max-1 chars) into `value`. Returns
 * 1 if found, 0 otherwise.
 */
static int find_header(const char *headers, const char *name,
                       char *value, size_t value_max) {
    size_t name_len = strlen(name);
    const char *p = headers;
    while (*p) {
        const char *line_end = strstr(p, "\r\n");
        if (!line_end) break;
        if ((size_t)(line_end - p) > name_len + 1 &&
            strncasecmp(p, name, name_len) == 0 &&
            p[name_len] == ':') {
            const char *v = p + name_len + 1;
            while (v < line_end && (*v == ' ' || *v == '\t')) v++;
            size_t vlen = (size_t)(line_end - v);
            if (vlen >= value_max) vlen = value_max - 1;
            memcpy(value, v, vlen);
            value[vlen] = '\0';
            return 1;
        }
        p = line_end + 2;
    }
    return 0;
}

static int compute_accept(const char *client_key, char *out, size_t out_max) {
    char concat[256];
    int n = snprintf(concat, sizeof(concat), "%s%s", client_key, WS_GUID);
    if (n < 0 || (size_t)n >= sizeof(concat)) return -1;

    sha1_ctx ctx;
    uint8_t digest[20];
    sha1_init(&ctx);
    sha1_update(&ctx, (const uint8_t*)concat, (size_t)n);
    sha1_final(digest, &ctx);

    /* base64-encode the 20-byte digest. libsodium needs a slightly
     * oversized output buffer; standard base64 of 20 bytes = 28 chars
     * + null terminator. */
    if (out_max < sodium_base64_ENCODED_LEN(20, sodium_base64_VARIANT_ORIGINAL)) {
        return -1;
    }
    sodium_bin2base64(out, out_max, digest, 20, sodium_base64_VARIANT_ORIGINAL);
    return 0;
}

int ws_server_handshake(int sock) {
    /* Read HTTP request headers up to the blank line. */
    char buf[2048];
    size_t total = 0;
    while (total < sizeof(buf) - 1) {
        ssize_t r = recv(sock, buf + total, sizeof(buf) - 1 - total, 0);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return -1;
        total += (size_t)r;
        buf[total] = '\0';
        if (strstr(buf, "\r\n\r\n")) break;
    }
    if (total >= sizeof(buf) - 1) return -1;

    char key[128];
    if (!find_header(buf, "Sec-WebSocket-Key", key, sizeof(key))) return -1;

    char accept[64];
    if (compute_accept(key, accept, sizeof(accept)) != 0) return -1;

    char resp[512];
    int n = snprintf(resp, sizeof(resp),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n",
        accept);
    if (n < 0 || (size_t)n >= sizeof(resp)) return -1;
    if (write_exact(sock, (const uint8_t*)resp, (size_t)n) != 0) return -1;
    return 0;
}

int ws_client_handshake(int sock, const char *host, int port) {
    /* Random 16-byte key, base64-encoded for the request header. */
    uint8_t key_bytes[16];
    randombytes_buf(key_bytes, sizeof(key_bytes));
    char key_b64[sodium_base64_ENCODED_LEN(16, sodium_base64_VARIANT_ORIGINAL)];
    sodium_bin2base64(key_b64, sizeof(key_b64), key_bytes, sizeof(key_bytes),
                      sodium_base64_VARIANT_ORIGINAL);

    char req[512];
    int n = snprintf(req, sizeof(req),
        "GET / HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n",
        host, port, key_b64);
    if (n < 0 || (size_t)n >= sizeof(req)) return -1;
    if (write_exact(sock, (const uint8_t*)req, (size_t)n) != 0) return -1;

    char buf[2048];
    size_t total = 0;
    while (total < sizeof(buf) - 1) {
        ssize_t r = recv(sock, buf + total, sizeof(buf) - 1 - total, 0);
        if (r < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (r == 0) return -1;
        total += (size_t)r;
        buf[total] = '\0';
        if (strstr(buf, "\r\n\r\n")) break;
    }
    if (total >= sizeof(buf) - 1) return -1;

    if (strncmp(buf, "HTTP/1.1 101", 12) != 0) return -1;
    return 0;
}

/* -------------------- Frame send / recv -------------------- */

int ws_send_binary(int sock, const uint8_t *payload, size_t len, int is_client) {
    if (len > WS_MAX_PAYLOAD) return -1;

    uint8_t header[14];
    size_t hlen = 0;
    header[hlen++] = 0x80 | WS_OP_BINARY;  /* FIN | opcode */

    if (len < 126) {
        header[hlen++] = (uint8_t)(is_client ? 0x80 : 0x00) | (uint8_t)len;
    } else {
        header[hlen++] = (uint8_t)(is_client ? 0x80 : 0x00) | 126;
        header[hlen++] = (uint8_t)((len >> 8) & 0xFF);
        header[hlen++] = (uint8_t)(len & 0xFF);
    }

    uint8_t mask[4] = {0};
    if (is_client) {
        randombytes_buf(mask, 4);
        memcpy(header + hlen, mask, 4);
        hlen += 4;
    }

    if (write_exact(sock, header, hlen) != 0) return -1;

    if (is_client && len > 0) {
        /* Send masked payload. To avoid mutating the caller's buffer or
         * allocating, do it in fixed-size chunks on the stack.
         */
        uint8_t chunk[256];
        size_t off = 0;
        while (off < len) {
            size_t take = len - off;
            if (take > sizeof(chunk)) take = sizeof(chunk);
            for (size_t i = 0; i < take; i++) {
                chunk[i] = payload[off + i] ^ mask[(off + i) & 3];
            }
            if (write_exact(sock, chunk, take) != 0) return -1;
            off += take;
        }
    } else if (len > 0) {
        if (write_exact(sock, payload, len) != 0) return -1;
    }

    return (int)(hlen + len);
}

/* Internal: read one frame, write payload into out (capacity max), set
 * *opcode_out to the frame opcode. Returns payload length on success,
 * -1 on error/EOF.
 */
static int recv_frame(int sock, uint8_t *out, size_t max, int *opcode_out) {
    uint8_t hdr2[2];
    if (read_exact(sock, hdr2, 2) != 0) return -1;

    int fin    = (hdr2[0] & 0x80) != 0;
    int opcode = hdr2[0] & 0x0F;
    int masked = (hdr2[1] & 0x80) != 0;
    uint64_t len = hdr2[1] & 0x7F;

    if (!fin) return -1;  /* we don't support fragmentation */

    if (len == 126) {
        uint8_t ext[2];
        if (read_exact(sock, ext, 2) != 0) return -1;
        len = ((uint64_t)ext[0] << 8) | (uint64_t)ext[1];
    } else if (len == 127) {
        uint8_t ext[8];
        if (read_exact(sock, ext, 8) != 0) return -1;
        len = 0;
        for (int i = 0; i < 8; i++) len = (len << 8) | ext[i];
    }
    if (len > max) return -1;

    uint8_t mask[4] = {0};
    if (masked) {
        if (read_exact(sock, mask, 4) != 0) return -1;
    }

    if (len > 0) {
        if (read_exact(sock, out, (size_t)len) != 0) return -1;
        if (masked) {
            for (uint64_t i = 0; i < len; i++) {
                out[i] ^= mask[i & 3];
            }
        }
    }

    *opcode_out = opcode;
    return (int)len;
}

int ws_recv_binary(int sock, uint8_t *out, size_t max_len) {
    for (;;) {
        int opcode = 0;
        int n = recv_frame(sock, out, max_len, &opcode);
        if (n < 0) return -1;

        switch (opcode) {
        case WS_OP_BINARY:
            return n;
        case WS_OP_PING: {
            /* Echo the payload back as a pong. RFC 6455 §5.5.2 says a
             * pong must repeat the ping's application data verbatim.
             */
            uint8_t header[4];
            size_t hlen = 0;
            header[hlen++] = 0x80 | WS_OP_PONG;
            if (n < 126) {
                header[hlen++] = (uint8_t)n;
            } else {
                header[hlen++] = 126;
                header[hlen++] = (uint8_t)((n >> 8) & 0xFF);
                header[hlen++] = (uint8_t)(n & 0xFF);
            }
            if (write_exact(sock, header, hlen) != 0) return -1;
            if (n > 0 && write_exact(sock, out, (size_t)n) != 0) return -1;
            break;  /* keep looping for the next frame */
        }
        case WS_OP_PONG:
            /* Unsolicited pong — ignore, keep reading. */
            break;
        case WS_OP_CLOSE:
            return -1;
        case WS_OP_TEXT:
            /* Not part of our protocol. Discard and continue. */
            break;
        default:
            return -1;
        }
    }
}
