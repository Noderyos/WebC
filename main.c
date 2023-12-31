#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include "request.h"

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

    int client_fd = accept(sockfd, NULL, NULL);
    if(client_fd < 0){
        perror("accept");
        return -1;
    }

    request req = {0};
    char response[1024];

    int header_idx = -1;

    char line[MAX_HEADER_SIZE+1];
    char buf[1];
    int line_idx = 0;
    while (0 != recv(client_fd, buf, 1, 0)){
        if(buf[0] == '\r'){
            recv(client_fd, buf, 1, 0);  // Skip \n after \r
            line[line_idx] = 0;

            if(line_idx == 0){
                printf("Breaking\n");
                break;
            }

            if(header_idx > MAX_HEADER_COUNT){
                strncpy(response, "HTTP/1.1 413 Payload Too Large\r\n\r\nPayload Too Large", 1023);
                goto send_request;
            }
            if (header_idx++ < 0){
                if(parse_before_header(&req, line) < 0){
                    strncpy(response, "HTTP/1.1 400 Bad Request\r\n\r\nBad Request\n", 1023);
                    goto send_request;
                }
            } else{
                if(parse_header(&req, line) < 0){
                    strncpy(response, "HTTP/1.1 400 Bad Request\r\n\r\nBad Request\n", 1023);
                    goto send_request;
                }
            }
            header_idx++;
            line_idx = 0;
        }else{
            if(line_idx > MAX_HEADER_SIZE){
                if(header_idx < 0)
                    strncpy(response, "HTTP/1.1 414 URI Too Long\r\n\r\nURI Too Long\n", 1023);
                else
                    strncpy(
                            response,
                            "HTTP/1.1 431 Request Header Fields Too Large\r\n\r\n"
                            "Request Header Fields Too Large\n",
                            1023);
                goto send_request;
            }
            line[line_idx++] = buf[0];
        }
    }
    handle_request(&req, response, 1023);

send_request:

    send(client_fd, response, strlen(response), 0);

    close(client_fd);
    close(sockfd);

    return 0;
}
