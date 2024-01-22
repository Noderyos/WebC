#include "websocket.h"

#define CHUNK 16384

int payload_inflate(unsigned char *payload, size_t payload_size, unsigned char **uncompressed_data, size_t *uncompressed_size){
    z_stream stream;
    int ret;
    size_t total_uncompressed_size = 0;
    size_t offset = 0;
    unsigned char out[CHUNK];

    memset(&stream, 0, sizeof(stream));
    if (inflateInit2(&stream, -15) != Z_OK) {
        return -1;
    }

    stream.next_in = (Bytef *)payload;
    stream.avail_in = (uInt)payload_size;

    do {
        stream.next_out = out;
        stream.avail_out = CHUNK;

        ret = inflate(&stream, Z_NO_FLUSH);
        if (ret < 0) {
            inflateEnd(&stream);
            return ret;
        }

        total_uncompressed_size += CHUNK - stream.avail_out;

        *uncompressed_data = realloc(*uncompressed_data, total_uncompressed_size);
        if (*uncompressed_data == NULL) {
            inflateEnd(&stream);
            return -1;
        }

        memcpy(*uncompressed_data + offset, out, CHUNK - stream.avail_out);
        offset += CHUNK - stream.avail_out;

    } while (stream.avail_out == 0);

    inflateEnd(&stream);

    *uncompressed_size = total_uncompressed_size;

    return 0;
}

ws_result ws_receive_preprocess(ws_packet* packet, int fd){
    uint8_t header[2];
    if(recv(fd, header, 2, 0) == 2){
        packet->hdr = malloc(sizeof(ws_header));

        memcpy(packet->hdr, header, 2);

        packet->real_packet_size = packet->hdr->payload_size;

        if(packet->hdr->payload_size == 126){
            uint8_t size[2];
            if(recv(fd, size, 2, 0) == 2){
                packet->real_packet_size = size[0] << 8 | size[1];
            }
        }else if(packet->hdr->payload_size == 127){
            uint8_t size[4];
            if(recv(fd, size, 4, 0) == 2){
                packet->real_packet_size =
                        (size[0] << 56) |
                        (size[1] << 48) |
                        (size[2] << 40) |
                        (size[3] << 32) |
                        (size[4] << 24) |
                        (size[5] << 16) |
                        (size[6] << 8) |
                        size[7];
            }
        }


        unsigned char* payload = (unsigned char*) malloc((unsigned int)packet->real_packet_size);
        memset(payload, 0, packet->real_packet_size);

        unsigned char mask[4] = {0};
        if(packet->hdr->mask)
            recv(fd, mask, 4, 0);

        recv(fd, payload, (unsigned int)packet->real_packet_size, 0);

        for (int i = 0; i < packet->real_packet_size; ++i)  // If the packet doesn't have mask, this will do nothing
            payload[i] ^= mask[i%4];

        size_t uncompressed_payload_size;

        payload_inflate(payload, packet->real_packet_size, &packet->payload, &uncompressed_payload_size);

        free(payload);
    }else{
        perror("recv");
        return RESULT_ERR;
    }
    return RESULT_OK;
}

void ws_cleanup(ws_packet* packet){
    free(packet->payload);
}

ws_result handle_ws_packet(ws_packet *packet, int fd){
    ws_header header = *packet->hdr;
    if(header.opcode == WS_PING){
        ws_header response_hdr = {0};
        response_hdr.fin = 1;
        response_hdr.mask = 0;
        response_hdr.reserved = 0;
        response_hdr.opcode = WS_PONG;
        response_hdr.payload_size = 0;
        send(fd, &response_hdr, sizeof response_hdr, 0);
        return RESULT_OK;
    }

    if(packet->hdr->opcode == WS_TEXT){
        ws_header response_hdr = {0};
        ws_packet send_packet = {0};

        response_hdr.fin = 1;
        response_hdr.mask = 0;
        response_hdr.reserved = 0;
        response_hdr.opcode = WS_TEXT;
        response_hdr.payload_size = 11;

        send_packet.hdr = &response_hdr;

        char * u = "Hello world";

        send_packet.payload = malloc(11);

        strcpy(send_packet.payload, u);

        ws_send_packet(&send_packet, fd);

        return RESULT_OK;
    }
    if(packet->hdr->opcode == WS_CONNECTION_CLOSE){
        ws_header response_hdr = {0};
        ws_packet send_packet = {0};
        response_hdr.fin = 1;
        response_hdr.mask = 0;
        response_hdr.reserved = 0;
        response_hdr.opcode = WS_CONNECTION_CLOSE;
        response_hdr.payload_size = 2;

        ws_close_code code = CLOSE_NORMAL;

        code = ((code & 0xff) << 8) | ((code & 0xff00) >> 8);

        send_packet.hdr = &response_hdr;
        send_packet.payload = (uint8_t*)&code;

        ws_send_packet(&send_packet, fd);
        return RESULT_ERR;
    }
    return RESULT_OK;
}

void ws_send_packet(ws_packet *packet, int fd){
    uint8_t buffer_size = 2;
    size_t buffer_idx = 0;

    if(packet->hdr->payload_size > 125){
        if(packet->hdr->payload_size == 126) buffer_size += 2;
        else if(packet->hdr->payload_size == 127) buffer_size += 4;
        buffer_size += packet->real_packet_size;
    }else{
        buffer_size += packet->hdr->payload_size;
    }

    uint8_t *send_buffer = (uint8_t *)malloc(buffer_size);
    memcpy(send_buffer, packet->hdr, 2);

    buffer_idx += 2;

    if(packet->hdr->payload_size > 125){
        if(packet->hdr->payload_size == 126) {
            memcpy(send_buffer + 2, &packet->real_packet_size, 2);
            buffer_idx += 2;
        }
        else if(packet->hdr->payload_size == 127) {
            memcpy(send_buffer + 2, &packet->real_packet_size, 4);
            buffer_idx += 4;
        }
        memcpy(send_buffer + buffer_idx, packet->payload, packet->real_packet_size);
    }else{
        memcpy(send_buffer + 2, packet->payload, packet->hdr->payload_size);
    }

    send(fd, send_buffer, buffer_size, 0);
    free(send_buffer);
}