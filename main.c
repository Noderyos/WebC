#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>

int main() {

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if(sockfd < 0){
        perror("socket");
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

    char* buf = "Hello world\n";

    send(client_fd, buf, strlen(buf), 0);

    close(client_fd);
    close(sockfd);

    return 0;
}
