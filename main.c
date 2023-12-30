#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <malloc.h>
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

    request r;

    char *req = (char*)malloc(1024);
    char *req_start = req;
    recv(client_fd, req, 1023, 0);

    int req_type_len = (int)(strchr(req, ' ')-req);

    r.method = (char*) malloc(req_type_len + 1);
    strncpy(r.method, req, req_type_len);

    req[req_type_len] = 0;
    req += req_type_len + 1;  // Move after request method


    int path_len = (int)(strchr(req, ' ') - req);

    r.path = (char*) malloc(path_len + 1);
    strncpy(r.path, req, path_len);

    req[path_len] = 0;
    req += path_len + 1;  // Move after request path

    char response[1024];

    handle_request(&r, response, 1023);

    send(client_fd, response, strlen(response), 0);

    free(req_start);
    free(r.method);
    free(r.path);

    close(client_fd);
    close(sockfd);

    return 0;
}
