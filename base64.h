#ifndef WEBC_BASE64_H
#define WEBC_BASE64_H

#include <string.h>
#include <malloc.h>
#include <stdint.h>


unsigned char* b64encode(char* input);
unsigned char* b64decode(char *input);

#endif