#ifndef WEBC_REQUEST_H
#define WEBC_REQUEST_H

#include <stdio.h>
#include <string.h>

typedef struct {
    char* method;
    char* path;
} request;

void handle_request(request *req, char* response, int max_rsp_len);

#endif //WEBC_REQUEST_H
