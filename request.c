#include "request.h"

void handle_request(request *req, char* response, int max_rsp_len){

    printf("User is %sing at %s\n", req->method, req->path);


    strncpy(response, "HTTP/1.1 200\r\nContent-Type: text/html\r\n\r\n", max_rsp_len);
    strncat(response, "You have requested the path ", max_rsp_len);
    strncat(response, req->path, max_rsp_len);
    strncat(response, " using a ", max_rsp_len);
    strncat(response, req->method, max_rsp_len);
    strncat(response, " request\n", max_rsp_len);

}