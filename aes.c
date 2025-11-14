#include "aes.h"
#include <string.h> // memcpy
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


static const uint8_t sbox[256] =
{
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5,
    0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0,
    0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc,
    0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a,
    0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0,
    0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b,
    0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85,
    0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5,
    0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17,
    0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88,
    0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c,
    0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9,
    0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6,
    0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e,
    0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94,
    0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68,
    0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};


static const uint8_t rcon[11] = {
    0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36
};


uint8_t x_times(uint8_t x) {
    // GF(2^8) multiplication in the Field
    // Shift left (this is multiplication by 2 before reduction)
    uint8_t shifted = x << 1;

    // If the original high bit was set, we must reduce with 0x1B
    if (x & 0x80) {
        return (shifted ^ 0x1B);
        // Apply Rijndael reduction polynomial, note normally we would use 11b but we due to the overflow from uint8 we dont need the 0x11b just 0x1b is enough
    }
    return shifted; // No reduction needed
}

void RotWord(uint8_t *word) {
    // four cells aka one row is swapped
    uint8_t temp = word[0];
    word[0] = word[1];
    word[1] = word[2];
    word[2] = word[3];
    word[3] = temp;
}

void SubBytes(uint8_t *block) {
    // block is per definition 16 long

    for (uint8_t i = 0; i < NUMBER_OF_COLUMNS; i++) {
        block[i] = sbox[block[i]]; // swap
    }
}

void MixColumns(uint8_t *block) {
    uint8_t a, b, c, d, e;
    for (uint8_t i = 0; i < BLOCK_SIZE; i += 4) {
        a = block[i];
        b = block[i + 1];
        c = block[i + 2];
        d = block[i + 3];
        e = a ^ b ^ c ^ d;
        block[i] ^= e ^ x_times(a ^ b);
        block[i + 1] ^= e ^ x_times(b ^ c);
        block[i + 2] ^= e ^ x_times(c ^ d);
        block[i + 3] ^= e ^ x_times(d ^ a);
    }
}


inline void insert_key_for_round(uint8_t round, uint8_t *key, uint8_t *round_keys_buffer) {
    // inserts the (round) key for the round position in the round keys buffer
    // eg. for round one we insert it at position 0 to 3
    // round two 4 to 8 in the round keys buffer
    uint8_t start = (round - 1) * 4;

    // Copy 4 bytes into the correct place
    memcpy(&round_keys_buffer[start], key, 4);
}


void gf_addition(uint8_t a[], uint8_t b[], uint8_t d[]) {
    // d = a + b in the gf2^8 field -> in this field addition is an XOR!
    d[0] = a[0] ^ b[0];
    d[1] = a[1] ^ b[1];
    d[2] = a[2] ^ b[2];
    d[3] = a[3] ^ b[3];
}

void expand_key(uint8_t *key, uint8_t *round_keys) {
    /*
     *fills the round keys array with the respective round keys
     * for round:
     * 1. its just the key itself
     * 2. yeah we need to use the algo
     * */

    uint8_t tmp[4];
    uint8_t i;
    uint8_t len = NUMBER_OF_COLUMNS * (NUMBER_OF_ROUNDS + 1);
    // first round key is just the round key itself
    for (i = 0; i < NUMBER_OF_32_WORDS_IN_KEY; i++) {
        round_keys[4 * i + 0] = key[4 * i + 0];
        round_keys[4 * i + 1] = key[4 * i + 1];
        round_keys[4 * i + 2] = key[4 * i + 2];
        round_keys[4 * i + 3] = key[4 * i + 3];
    }
    // all other round keys need to be calculated
    for (i = NUMBER_OF_32_WORDS_IN_KEY; i < len; i++) {
        tmp[0] = round_keys[4 * (i - 1) + 0];
        tmp[1] = round_keys[4 * (i - 1) + 1];
        tmp[2] = round_keys[4 * (i - 1) + 2];
        tmp[3] = round_keys[4 * (i - 1) + 3];

        if (i % NUMBER_OF_32_WORDS_IN_KEY == 0) {
            RotWord(tmp);
            SubBytes(tmp);
            gf_addition(tmp, &rcon[i / NUMBER_OF_COLUMNS], tmp);
        }
        //else if (NUMBER_OF_32_WORDS_IN_KEY > 6 && i%NUMBER_OF_ROUNDS == 4) {
        //
        //     sub_word(tmp);
        //
        // }

        round_keys[4 * i + 0] = round_keys[4 * (i - NUMBER_OF_32_WORDS_IN_KEY) + 0] ^ tmp[0];
        round_keys[4 * i + 1] = round_keys[4 * (i - NUMBER_OF_32_WORDS_IN_KEY) + 1] ^ tmp[1];
        round_keys[4 * i + 2] = round_keys[4 * (i - NUMBER_OF_32_WORDS_IN_KEY) + 2] ^ tmp[2];
        round_keys[4 * i + 3] = round_keys[4 * (i - NUMBER_OF_32_WORDS_IN_KEY) + 3] ^ tmp[3];
    }
}

uint8_t *aes128_init(void *key) {
    //creates the round keys, might fail tho!

    uint8_t *round_keys = malloc(NUMBER_OF_COLUMNS * (NUMBER_OF_ROUNDS + 1) * 4);
    if (round_keys == NULL) {
        printf("malloc failed\n");
        //TODO handle this
        return NULL; // :) :(
    }
    expand_key(key, round_keys);
    return round_keys;
}


void aes128_encrypt(void *buffer, void *round_keys) {
    // buffer: the buffer which contains the cleartext/message which shall be encrypted



}
