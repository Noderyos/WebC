#ifndef WEBC_HTTP_H
#define WEBC_HTTP_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>


#define MAX_HEADER_NAME_SIZE 40
#define MAX_HEADER_SIZE 4096 // HTTP RFC doesn't give a limit
#define MAX_HEADER_COUNT 64
#define MAX_URL_SIZE 2048

typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_PATCH,
    HTTP_HEAD,
    HTTP_OPTIONS,
    HTTP_TRACE,
    HTTP_UNKNOWN
} http_method;

typedef enum {
    HTTP_CONNECTION,
    WEBSOCKET_CONNECTION
} connection_type;

typedef struct {
    char _data[MAX_HEADER_SIZE];
    char* name;
    char* value;
} header;

typedef struct {
    http_method method;
    char path[MAX_URL_SIZE];
    int header_count;
    header headers[MAX_HEADER_COUNT];
    char *response;
} request;

void handle_request(request *req, int max_rsp_len);

int parse_before_header(request *req, char* line);
int parse_header(request *req, char* line);

char* get_header(request *req, char* name);

#endif //WEBC_HTTP_H
