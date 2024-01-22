#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include "http.h"
#include "base64.h"
#include "sha1.h"
#include "websocket.h"

#define MAX_RESP_SIZE 1024


typedef enum {
    HTTP_CONNECTION,
    WEBSOCKET_CONNECTION
} connection_type;

int parse_http_headers(int fd, http_request *req);
char* should_upgrade(http_request *req);
void strcat_format(char *destination, const char *format, ...);

int main() {

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd < 0){
        perror("socket");
        return -1;
    }

    int o = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &o, sizeof(o)) < 0){
        perror("setsockopt");
        return -1;
    }

    struct sockaddr_in sock_info = {0};
    sock_info.sin_family = AF_INET;
    sock_info.sin_port = htons(8080);
    sock_info.sin_addr.s_addr = 0x0;

    if(bind(sockfd, (struct sockaddr *)&sock_info, sizeof(sock_info)) < 0){
        perror("bind");
        return -1;
    }

    if(listen(sockfd, 10)){ // max 10 clients in queue
        perror("listen");
        return -1;
    }


    int clientfd = accept(sockfd, NULL, NULL);
    if(clientfd < 0){
        perror("accept");
        return -1;
    }

    connection_type type = HTTP_CONNECTION;
    int should_close = 0;

    while (!should_close){
        if(type == HTTP_CONNECTION){
            http_request req = {0};
            char response[MAX_RESP_SIZE+1] = {0};

            req.response = response;

            if(parse_http_headers(clientfd, &req) == 0){
                char* upgrade = should_upgrade(&req);
                if(upgrade){
                    if(strcmp(upgrade, "websocket") == 0){
                        char* ws_key = get_header(&req, "Sec-WebSocket-Key");
                        if(ws_key){
                            char key[1024] = {0}, hash[21] = {0};
                            strcpy(key, ws_key);
                            strcat(key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

                            SHA1((uint8_t *)key, (uint8_t *)hash, strlen(key));
                            unsigned char* accept = b64encode(hash);

                            strcat_format(req.response,
                                          "HTTP/1.1 101 Switching Protocols\r\n"
                                          "Upgrade: websocket\r\n"
                                          "Connection: Upgrade\r\n"
                                          "Sec-WebSocket-Accept: %s\r\n"
                                          "Sec-WebSocket-Extensions: permessage-deflate; server_max_window_bits=12; client_max_window_bits=12\r\n\r\n",
                                          accept);
                            free(accept);
                            type = WEBSOCKET_CONNECTION;
                        }else{
                            strncpy(req.response,
                                    "HTTP/1.1 400 Bad Request\r\n"
                                    "\r\n"
                                    "Bad Request\n", MAX_RESP_SIZE);
                        }
                    }
                }else{
                    handle_request(&req, MAX_RESP_SIZE);
                    should_close = 1;
                }
            }
            send(clientfd, response, strlen(response), 0);
        }else if(type == WEBSOCKET_CONNECTION){
            ws_packet packet = {0};
            if(ws_receive_preprocess(&packet, clientfd) == RESULT_ERR){
                should_close = 1;
                continue;
            }
            if(handle_ws_packet(&packet, clientfd) == RESULT_ERR){
                should_close = 1;
                continue;
            }
            ws_cleanup(&packet);
        }
    }

    close(clientfd);
    close(sockfd);

    return 0;
}

int parse_http_headers(int fd, http_request *req){
    int header_idx = -1;

    char line[MAX_HEADER_SIZE+1];
    char buf[1];
    int line_idx = 0;
    while (0 != recv(fd, buf, 1, 0)){
        if(buf[0] == '\r'){
            recv(fd, buf, 1, 0);  // Skip \n after \r
            line[line_idx] = 0;

            if(line_idx == 0)
                break;

            if(header_idx > MAX_HEADER_COUNT){
                strncpy(req->response, "HTTP/1.1 413 Payload Too Large\r\n\r\nPayload Too Large", MAX_RESP_SIZE);
                return -1;
            }
            if (header_idx++ < 0){
                if(parse_before_header(req, line) < 0){
                    strncpy(req->response, "HTTP/1.1 400 Bad Request\r\n\r\nBad Request\n", MAX_RESP_SIZE);
                    return -1;
                }
            } else{
                if(parse_header(req, line) < 0){
                    strncpy(req->response, "HTTP/1.1 400 Bad Request\r\n\r\nBad Request\n", MAX_RESP_SIZE);
                    return -1;
                }
            }
            header_idx++;
            line_idx = 0;
        }else{
            if(line_idx > MAX_HEADER_SIZE){
                if(header_idx < 0)
                    strncpy(req->response, "HTTP/1.1 414 URI Too Long\r\n\r\nURI Too Long\n", MAX_RESP_SIZE);
                else
                    strncpy(
                            req->response,
                            "HTTP/1.1 431 Request Header Fields Too Large\r\n\r\n"
                            "Request Header Fields Too Large\n",
                            MAX_RESP_SIZE);
                return -1;
            }
            line[line_idx++] = buf[0];
        }
    }
    return 0;
}

char* should_upgrade(http_request *req){
    if(get_header(req, "Connection")){
        if(strcmp(get_header(req, "Connection"), "Upgrade") == 0){
            char* upgrade = get_header(req, "Upgrade");
            if(upgrade){
                return upgrade;
            }
        }
    }
    return NULL;
}

void strcat_format(char *destination, const char *format, ...) {
    va_list args;
    va_start(args, format);

    char* formated = (char*) malloc(1024);
    vsprintf(formated, format, args);

    va_end(args);
    strcat(destination, formated);
    free(formated);
}