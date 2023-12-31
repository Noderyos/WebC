#ifndef SHA1_H
#define SHA1_H

#include <stdint.h>

#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32-(c))))

typedef struct SHA1Context
{
    uint32_t K[5];
    int_least16_t blk_idx;
    uint8_t Message_Block[64];
} SHA1Context;

void SHA1(const uint8_t *message_array, uint8_t *message_digest, unsigned length);

#endif