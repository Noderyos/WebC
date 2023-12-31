#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include "include/http.h"
#include "include/base64.h"
#include "include/sha1.h"

#define MAX_RESP_SIZE 1024

int parse_headers(int fd, request *req);
char* should_upgrade(request *req);
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
            request req = {0};
            char response[MAX_RESP_SIZE+1] = {0};

            req.response = response;

            if(parse_headers(clientfd, &req) == 0){
                char* upgrade = should_upgrade(&req);
                if(upgrade){
                    if(strcmp(upgrade, "websocket") == 0){
                        char* ws_key = get_header(&req, "Sec-WebSocket-Key");
                        if(ws_key){
                            char key[1024] = {0}, hash[21] = {0};
                            strcpy(key, ws_key);
                            strcat(key, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

                            SHA1(key, hash, strlen(key));
                            char* accept = b64encode(hash);

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
                            strncpy(req.response, "HTTP/1.1 400 Bad Request\r\n\r\nBad Request\n", MAX_RESP_SIZE);
                        }
                    }
                }else{
                    handle_request(&req, MAX_RESP_SIZE);
                    should_close = 1;
                }
            }
            send(clientfd, response, strlen(response), 0);
        }else if(type == WEBSOCKET_CONNECTION){
            char bb[1024];
            int a = recv(clientfd, bb, 1023, 0);
            if(a){
                for (int i = 0; i < a; ++i) {
                    printf("%02x", bb[i] & 0xff);
                }
                printf("\n");
            }
        }
    }

    close(clientfd);
    close(sockfd);

    return 0;
}

int parse_headers(int fd, request *req){
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

char* should_upgrade(request *req){
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