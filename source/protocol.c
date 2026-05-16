
#include "protocol.h"

void proto_handshake_encode(uint8_t *buf, const proto_handshake_t *h) {
    buf[0]  = PROTO_MAGIC_0;
    buf[1]  = PROTO_MAGIC_1;
    buf[2]  = PROTO_MAGIC_2;
    buf[3]  = PROTO_MAGIC_3;
    buf[4]  = (uint8_t)(h->version >> 8);
    buf[5]  = (uint8_t)(h->version & 0xFF);
    buf[6]  = (uint8_t)(h->max_clients >> 8);
    buf[7]  = (uint8_t)(h->max_clients & 0xFF);
    buf[8]  = (uint8_t)(h->heartbeat_ms >> 24);
    buf[9]  = (uint8_t)(h->heartbeat_ms >> 16);
    buf[10] = (uint8_t)(h->heartbeat_ms >> 8);
    buf[11] = (uint8_t)(h->heartbeat_ms & 0xFF);
    buf[12] = (uint8_t)(h->client_packet_size >> 24);
    buf[13] = (uint8_t)(h->client_packet_size >> 16);
    buf[14] = (uint8_t)(h->client_packet_size >> 8);
    buf[15] = (uint8_t)(h->client_packet_size & 0xFF);
}

int proto_handshake_decode(const uint8_t *buf, proto_handshake_t *h) {
    if (buf[0] != PROTO_MAGIC_0 ||
        buf[1] != PROTO_MAGIC_1 ||
        buf[2] != PROTO_MAGIC_2 ||
        buf[3] != PROTO_MAGIC_3) {
        return -1;
    }
    h->version            = ((uint16_t)buf[4] << 8) | buf[5];
    if (h->version != PROTO_VERSION) {
        return -1;
    }
    h->max_clients        = ((uint16_t)buf[6] << 8) | buf[7];
    h->heartbeat_ms       = ((uint32_t)buf[8]  << 24) |
                            ((uint32_t)buf[9]  << 16) |
                            ((uint32_t)buf[10] << 8)  |
                             (uint32_t)buf[11];
    h->client_packet_size = ((uint32_t)buf[12] << 24) |
                            ((uint32_t)buf[13] << 16) |
                            ((uint32_t)buf[14] << 8)  |
                             (uint32_t)buf[15];
    return 0;
}
