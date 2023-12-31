#include "include/base64.h"

unsigned char* b64encode(char* input){

    char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    int input_length = (int)strlen(input);

    int output_len = 4 * ((input_length + 2) / 3);

    unsigned char *encoded_data = malloc(output_len);
    if (encoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)input[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)input[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)input[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = alphabet[(triple >> 3 * 6) & 63];
        encoded_data[j++] = alphabet[(triple >> 2 * 6) & 63];
        encoded_data[j++] = alphabet[(triple >> 1 * 6) & 63];
        encoded_data[j++] = alphabet[(triple >> 0 * 6) & 63];
    }

    int mod_table[] = {0, 2, 1};

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[output_len - 1 - i] = '=';

    return encoded_data;
}

unsigned char* b64decode(char *input) {

    char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char* decoding_table = (char*)malloc(256);

    for (int i = 0; i < 64; i++)
        decoding_table[(unsigned char) alphabet[i]] = (char)i;

    int input_len = (int)strlen(input);

    if (input_len % 4 != 0) return NULL;

    int output_len = input_len / 4 * 3;
    if (input[input_len - 1] == '=') output_len--;
    if (input[input_len - 2] == '=') output_len--;

    unsigned char *decoded_data = malloc(output_len);
    if (decoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_len;) {
        uint32_t a = input[i] == '=' ? 0 & i++ : decoding_table[(int)input[i++]];
        uint32_t b = input[i] == '=' ? 0 & i++ : decoding_table[(int)input[i++]];
        uint32_t c = input[i] == '=' ? 0 & i++ : decoding_table[(int)input[i++]];
        uint32_t d = input[i] == '=' ? 0 & i++ : decoding_table[(int)input[i++]];

        uint32_t triple = (a << 3 * 6) + (b << 2 * 6) + (c << 1 * 6) + (d << 0 * 6);

        if (j < output_len) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < output_len) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < output_len) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}