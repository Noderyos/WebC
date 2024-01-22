#ifndef WEBC_WEBSOCKET_H
#define WEBC_WEBSOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <zlib.h>
#include <sys/socket.h>

typedef enum{
    RESULT_OK,
    RESULT_ERR
} ws_result;

typedef enum{
    WS_CONTINUATION,
    WS_TEXT,
    WS_BINARY,
    WS_CONNECTION_CLOSE = 8,
    WS_PING,
    WS_PONG
} ws_opcode;

typedef enum {
    CLOSE_NORMAL = 1000,  // The connection was closed normally, without error.
    CLOSE_GOING_AWAY,  // The endpoint is going away, typically indicating a server-side shutdown.
    CLOSE_PROTOCOL_ERR,  // A protocol error was detected.
    CLOSE_UNSUPPORTED_DATA,  // The server received inconsistent or malformed data.
    CLOSE_NO_STATUS = 1005,  // No status code was present in the closing frame.
    CLOSE_ABNORMAL,  // The connection was closed abnormally, for example, without sending a valid closing frame.
    CLOSE_INVALID_PAYLOAD,  // The server received data that it cannot accept.
    CLOSE_POLICY_VIOLATION,  // The connection was closed due to a violation of security policy.
    CLOSE_MESSAGE_TOO_BIG,  // The server received a message too large to process.
    CLOSE_MISSING_EXTENSION,  // The client requested one or more extensions that are not supported by the server.
    CLOSE_INTERNAL_ERROR,  // The server encountered an unexpected condition that prevented it from fulfilling the http_request.
    CLOSE_SERVICE_RESTART,  // The server is restarting.
    CLOSE_TRY_AGAIN_LATER,  // The server is temporarily unavailable, often due to server overload or maintenance.
    CLOSE_BAD_GATEWAY,  // The server is acting as a gateway or proxy and received an invalid response from the upstream server.
    CLOSE_TLS_HANDSHAKE_FAILURE,  // The closing frame has a reserved status code and cannot be used. This may occur if the connection uses TLS encryption and the handshake process fails.
} ws_close_code;



typedef struct {
    unsigned int opcode : 4;
    unsigned int reserved : 3;
    unsigned int fin : 1;
    unsigned int payload_size : 7;
    unsigned int mask : 1;
} ws_header;


typedef struct {
    ws_header *hdr;
    uint64_t real_packet_size;
    uint8_t *payload;
} ws_packet;

ws_result ws_receive_preprocess(ws_packet* packet, int fd);
void ws_cleanup(ws_packet* packet);

int payload_inflate(unsigned char *payload, size_t payload_size, unsigned char **uncompressed_data, size_t *uncompressed_size);

ws_result handle_ws_packet(ws_packet *packet, int fd);

void ws_send_packet(ws_packet *packet, int fd);

#endif //WEBC_WEBSOCKET_H
