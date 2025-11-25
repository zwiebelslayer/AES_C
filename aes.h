
#ifndef AES_C_AES_H
#define AES_C_AES_H
#include <stdint.h>
#define BLOCK_SIZE 16 // 128 bit @ 8bit per cell ==> 16 cells => Block, often printed as a table in books
#define KEY_SIZE 16 // key has 16 entires
#define NUMBER_OF_COLUMNS 4
#define NUMBER_OF_ROWS 4
#define NUMBER_OF_ROUNDS 10 // NR idk tbh depends on what aes you use
#define assert(expression)
#define USE_TBOX_IMPL 0

uint8_t x_times(uint8_t x);
void RotWord(uint8_t*);
void SubBytes(uint8_t*);
void MixColumns(uint8_t*);
void gf_addition(const uint8_t a[], const uint8_t b[], uint8_t d[]);
void expand_key(uint8_t *key, uint8_t *round_keys);
uint8_t *aes128_init(void *key);
void aes128_encrypt(void *buffer, void *round_keys);
void add_round_key(uint8_t *, uint8_t *, uint8_t);
void ShiftRows(uint8_t *state);
uint8_t gf_multiply(uint8_t a, uint8_t b);
void aes128_encrypt_classic(void *buffer, void *round_keys);


// t box stuff
#if USE_TBOX_IMPL == 1
void aes128_encrypt_tbox(void *buffer, void *round_keys);
void compute_t_boxes();


#endif
#endif //AES_C_AES_H