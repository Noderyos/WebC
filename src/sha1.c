#include "../include/sha1.h"

void process_msg_block(SHA1Context *context){
    int t;
    uint32_t W[80];

    for(t = 0; t < 16; t++){
        W[t]  = (uint32_t)context->Message_Block[t * 4] << 24;
        W[t] |= (uint32_t)context->Message_Block[t * 4 + 1] << 16;
        W[t] |= (uint32_t)context->Message_Block[t * 4 + 2] << 8;
        W[t] |= (uint32_t)context->Message_Block[t * 4 + 3];
    }

    for(t = 16; t < 80; t++)
        W[t] = LEFTROTATE(W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16], 1);

    uint32_t a = context->K[0];
    uint32_t b = context->K[1];
    uint32_t c = context->K[2];
    uint32_t d = context->K[3];
    uint32_t e = context->K[4];

    uint32_t f;
    uint32_t k;

    for (int i = 0; i < 80; i++){
        if(i <= 19){
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        }else if(i <= 39){
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        }else if(i <= 59){
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        }else{
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }
        uint32_t temp = LEFTROTATE(a, 5) + f + e + k + W[i];
        e = d;
        d = c;
        c = LEFTROTATE(b, 30);
        b = a;
        a = temp;
    }
    context->K[0] += a;
    context->K[1] += b;
    context->K[2] += c;
    context->K[3] += d;
    context->K[4] += e;

    context->blk_idx = 0;
}

void SHA1(const uint8_t *message_array, uint8_t *message_digest, unsigned length){
    
    SHA1Context context;

    uint32_t length_low = 0;
    uint32_t length_high = 0;
    context.blk_idx = 0;

    context.K[0] = 0x67452301;
    context.K[1] = 0xEFCDAB89;
    context.K[2] = 0x98BADCFE;
    context.K[3] = 0x10325476;
    context.K[4] = 0xC3D2E1F0;

    while(length--){
        context.Message_Block[context.blk_idx++] = *message_array;

        length_low += 8;
        if (length_low == 0)
            length_high++;

        if (context.blk_idx == 64)
            process_msg_block(&context);

        message_array++;
    }

    context.Message_Block[context.blk_idx++] = 0x80;

    if(context.blk_idx > 55){
        while(context.blk_idx < 64)
            context.Message_Block[context.blk_idx++] = 0;


        process_msg_block(&context);
    }

    while(context.blk_idx < 56)
        context.Message_Block[context.blk_idx++] = 0;


    context.Message_Block[56] = length_high >> 24;
    context.Message_Block[57] = length_high >> 16;
    context.Message_Block[58] = length_high >> 8;
    context.Message_Block[59] = length_high;
    context.Message_Block[60] = length_low >> 24;
    context.Message_Block[61] = length_low >> 16;
    context.Message_Block[62] = length_low >> 8;
    context.Message_Block[63] = length_low;

    process_msg_block(&context);

    for(int i = 0; i < 64; ++i)
        context.Message_Block[i] = 0;

    for(int i = 0; i < 20; ++i)
        message_digest[i] = context.K[i >> 2] >> 8 * (3-(i&0x03));
}