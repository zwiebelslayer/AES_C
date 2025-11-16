#include <stdio.h>
#include <inttypes.h>
#include "aes.h"

//static uint8_t key[16] =
//        {0xF3, 0x54, 0x1F, 0xA3, 0x4B, 0x33, 0x9C, 0x0D,
//         0x80, 0x23, 0x7A, 0xF9, 0x7C, 0x21, 0xD7, 0x3B}; // this is the key to encrypt it
//static uint8_t buf[16] =
//        {0x83, 0x85, 0x1F, 0xAB, 0x60, 0x41, 0xCD, 0xF5,
//         0x4A, 0x41, 0x6C, 0xDA, 0xF0, 0x12, 0xC2, 0xD4}; // this will be encrypted
//


void print_block(const char *label, const uint8_t *block) {
    printf("%s: ", label);
    for (int i = 0; i < 16; i++) {
        printf("%02X ", block[i]);
    }
    printf("\n");
}


int main(void) {
    static uint8_t test_vector[16] = {
            0x00, 0x11, 0x22, 0x33,
            0x44, 0x55, 0x66, 0x77,
            0x88, 0x99, 0xAA, 0xBB,
            0xCC, 0xDD, 0xEE, 0xFF
    };
    static uint8_t test_vector_key[16] = {
            0x00, 0x01, 0x02, 0x03,
            0x04, 0x05, 0x06, 0x07,
            0x08, 0x09, 0x0A, 0x0B,
            0x0C, 0x0D, 0x0E, 0x0F

    };
    print_block("Plaintext", test_vector);
    print_block("Key", test_vector_key);

    uint8_t *init_keys = aes128_init(test_vector_key);
    print_block("Init Keys", init_keys + 32);

    aes128_encrypt(test_vector, init_keys);
    print_block("Ciphertext", test_vector);
    return 0;
}