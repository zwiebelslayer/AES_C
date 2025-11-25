#include "aes.h"

#include <stdbool.h>
#include <string.h> // memcpy

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


static const uint8_t rcon[10][4] = {
    {0x01, 0x00, 0x00, 0x00},
    {0x02, 0x00, 0x00, 0x00},
    {0x04, 0x00, 0x00, 0x00},
    {0x08, 0x00, 0x00, 0x00},
    {0x10, 0x00, 0x00, 0x00},
    {0x20, 0x00, 0x00, 0x00},
    {0x40, 0x00, 0x00, 0x00},
    {0x80, 0x00, 0x00, 0x00},
    {0x1b, 0x00, 0x00, 0x00},
    {0x36, 0x00, 0x00, 0x00}
};

#if USE_TBOX_IMPL == 1
// for a very speedy implementation!
// 32bit = one row, we need bitshifts for lookup later on
uint32_t TBox0[256];
uint32_t TBox1[256];
uint32_t TBox2[256];
uint32_t TBox3[256];

uint32_t concat_row(uint8_t b3, uint8_t b2, uint8_t b1, uint8_t b0) {
    // concatenate the 4x8 into one 32
    uint32_t result = 0;

    result |= ((uint32_t) b3 << 24);
    result |= ((uint32_t) b2 << 16);
    result |= ((uint32_t) b1 << 8);
    result |= ((uint32_t) b0);
    return result;
}

void compute_t_boxes() {
    uint8_t s; // S-Box output for the current index i

    for (int i = 0; i < 256; i++) {
        s = sbox[i];

        // The coefficients (02, 03, 01, 01) are applied to the S-Box output (s),
        // and then cyclically shifted (representing ShiftRows and MixColumns).

        // TBox0: Combines MixColumn Row 0: [02, 03, 01, 01] and ShiftRow 0 (no shift)
        // [ b3, b2, b1, b0 ] = [ (02*s), (01*s), (01*s), (03*s) ]
        TBox0[i] = concat_row(
            gf_multiply(s, 0x02), // Contributes to State[0, c]
            gf_multiply(s, 0x01), // Contributes to State[1, c]
            gf_multiply(s, 0x01), // Contributes to State[2, c]
            gf_multiply(s, 0x03) // Contributes to State[3, c]
        );

        // TBox1: Combines MixColumn Row 1: [01, 02, 03, 01] and ShiftRow 1 (1 byte shift)
        // [ b3, b2, b1, b0 ] = [ (03*s), (02*s), (01*s), (01*s) ] (after ShiftRows/MixColumns adjustment)
        TBox1[i] = concat_row(
            gf_multiply(s, 0x03), // Contributes to State[0, c]
            gf_multiply(s, 0x02), // Contributes to State[1, c]
            gf_multiply(s, 0x01), // Contributes to State[2, c]
            gf_multiply(s, 0x01) // Contributes to State[3, c]
        );

        // TBox2: Combines MixColumn Row 2: [01, 01, 02, 03] and ShiftRow 2 (2 byte shift)
        // [ b3, b2, b1, b0 ] = [ (01*s), (03*s), (02*s), (01*s) ]
        TBox2[i] = concat_row(
            gf_multiply(s, 0x01), // Contributes to State[0, c]
            gf_multiply(s, 0x03), // Contributes to State[1, c]
            gf_multiply(s, 0x02), // Contributes to State[2, c]
            gf_multiply(s, 0x01) // Contributes to State[3, c]
        );

        // TBox3: Combines MixColumn Row 3: [03, 01, 01, 02] and ShiftRow 3 (3 byte shift)
        // [ b3, b2, b1, b0 ] = [ (01*s), (01*s), (03*s), (02*s) ]
        TBox3[i] = concat_row(
            gf_multiply(s, 0x01), // Contributes to State[0, c]
            gf_multiply(s, 0x01), // Contributes to State[1, c]
            gf_multiply(s, 0x03), // Contributes to State[2, c]
            gf_multiply(s, 0x02) // Contributes to State[3, c]
        );
    }
}


void tbox_lookup_round(uint8_t *state) {
    uint32_t e0, e1, e2, e3;
    // combination of sub bytes, shift rows, mix columsn in one via the t box

    e0 = TBox0[state[0]] ^ TBox1[state[5]] ^ TBox2[state[10]] ^ TBox3[state[15]];

    e1 = TBox0[state[4]] ^ TBox1[state[9]] ^ TBox2[state[14]] ^ TBox3[state[3]];


    e2 = TBox0[state[8]] ^ TBox1[state[13]] ^ TBox2[state[2]] ^ TBox3[state[7]];

    e3 = TBox0[state[12]] ^ TBox1[state[1]] ^ TBox2[state[6]] ^ TBox3[state[11]];
    // Row 0
    state[0] = (uint8_t) (e0 >> 24);
    state[1] = (uint8_t) (e0 >> 16);
    state[2] = (uint8_t) (e0 >> 8);
    state[3] = (uint8_t) e0;
    // Row 1
    state[4] = (uint8_t) (e1 >> 24);
    state[5] = (uint8_t) (e1 >> 16);
    state[6] = (uint8_t) (e1 >> 8);
    state[7] = (uint8_t) e1;

    // Row 2
    state[8] = (uint8_t) (e2 >> 24);
    state[9] = (uint8_t) (e2 >> 16);
    state[10] = (uint8_t) (e2 >> 8);
    state[11] = (uint8_t) e2;

    // Row 3
    state[12] = (uint8_t) (e3 >> 24);
    state[13] = (uint8_t) (e3 >> 16);
    state[14] = (uint8_t) (e3 >> 8);
    state[15] = (uint8_t) e3;
}

void aes128_encrypt_tbox(void *buffer, void *round_keys) {
    uint8_t *input_buffer = (uint8_t *) buffer;
    uint8_t state[BLOCK_SIZE];

    // 1. Initial Load and AddRoundKey
    memcpy(state, input_buffer, BLOCK_SIZE);
    print_block_two("Input before add round key", state);

    // Round 0 (Initial AddRoundKey)
    add_round_key(state, round_keys, 0);
    print_block_two("After initial AddRoundKey (Round 0)", state);

    // 2. Main Rounds (Rounds 1 through 9)
    for (uint8_t round = 1; round < NUMBER_OF_ROUNDS; round++) {
        // T-BOX OPTIMIZATION: Replaces SubBytes, ShiftRows, and MixColumns
        tbox_lookup_round(state);
        print_block_two("After T-Box Lookup", state);

        add_round_key(state, round_keys, round);
        print_block_two("After AddRoundKey (Round %d)", state);
    }

    // 3. Final Round (Round 10) - No MixColumns
    // Must use the separate functions here
    SubBytes(state);
    print_block_two("Final SubBytes", state);

    ShiftRows(state);
    print_block_two("Final ShiftRows", state);

    add_round_key(state, round_keys, NUMBER_OF_ROUNDS);
    print_block_two("Final AddRoundKey", state);

    // 4. Copy State back to Output Buffer
    memcpy(input_buffer, state, BLOCK_SIZE);
}



#endif





uint8_t gf_multiply(uint8_t a, uint8_t b) {
    uint8_t product = 0;
    uint8_t carry;
    uint8_t temp_a = a;

    // 0x02 and 0x03 for the mix column operation
    if (b == 0x02) {
        carry = (temp_a & 0x80) ? 0x1B : 0x00;
        product = (temp_a << 1) ^ carry;
    } else if (b == 0x03) {
        // 0x03 * a = (0x02 * a) XOR a = zwei mal a plus a sozusagen
        uint8_t a_times_2;
        carry = (temp_a & 0x80) ? 0x1B : 0x00;
        a_times_2 = (temp_a << 1) ^ carry;
        product = a_times_2 ^ temp_a;
    } else if (b == 0x01) {
        product = a;
    } else {
        //safety fallback, becauese more is not implemented!
        product = a;
    }
    return product;
}

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

inline void RotWord(uint8_t *word) {
    // four cells aka one row is swapped
    uint8_t temp = word[0];
    word[0] = word[1];
    word[1] = word[2];
    word[2] = word[3];
    word[3] = temp;
}

inline void SubBytes(uint8_t *block) {
    // block is per definition 16 long

    for (uint8_t i = 0; i < 16; i++) {
        block[i] = sbox[block[i]]; // swap
    }
}

inline void MixColumns(uint8_t *block) {
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


void gf_addition(const uint8_t a[], const uint8_t b[], uint8_t d[]) {
    // d = a + b in the gf2^8 field -> in this field addition is an XOR!
    d[0] = a[0] ^ b[0];
    d[1] = a[1] ^ b[1];
    d[2] = a[2] ^ b[2];
    d[3] = a[3] ^ b[3];
}

static inline void SubWord(uint8_t w[4]) {
    w[0] = sbox[w[0]];
    w[1] = sbox[w[1]];
    w[2] = sbox[w[2]];
    w[3] = sbox[w[3]];
}

void expand_key(uint8_t *key, uint8_t *round_keys) {
    uint8_t tmp[4];
    uint8_t i;

    const uint8_t number_of_words_in_key = 4; // 4, 6, or 8

    // we need 44 words for all round keys,
    uint8_t total_words = 44;

    // fill the round keys with the initial master key as the start point
    for (i = 0; i < number_of_words_in_key; i++) {
        round_keys[4 * i + 0] = key[4 * i + 0];
        round_keys[4 * i + 1] = key[4 * i + 1];
        round_keys[4 * i + 2] = key[4 * i + 2];
        round_keys[4 * i + 3] = key[4 * i + 3];
    }

    // Expand the rest
    for (i = number_of_words_in_key; i < total_words; i++) {
        // tmp = previous word
        tmp[0] = round_keys[4 * (i - 1) + 0];
        tmp[1] = round_keys[4 * (i - 1) + 1];
        tmp[2] = round_keys[4 * (i - 1) + 2];
        tmp[3] = round_keys[4 * (i - 1) + 3];

        if (i % number_of_words_in_key == 0) {
            RotWord(tmp); // rotate 4 bytes [b0 b1 b2 b3] -> [b1 b2 b3 b0]
            SubWord(tmp);
            gf_addition(tmp, rcon[(i / number_of_words_in_key) - 1], tmp);
        }


        // w[i] = w[i - Nk] ^ tmp
        round_keys[4 * i + 0] = round_keys[4 * (i - number_of_words_in_key) + 0] ^ tmp[0];
        round_keys[4 * i + 1] = round_keys[4 * (i - number_of_words_in_key) + 1] ^ tmp[1];
        round_keys[4 * i + 2] = round_keys[4 * (i - number_of_words_in_key) + 2] ^ tmp[2];
        round_keys[4 * i + 3] = round_keys[4 * (i - number_of_words_in_key) + 3] ^ tmp[3];
    }
}

uint8_t *aes128_init(void *key) {
    //creates the round keys, might fail tho!

    uint8_t *round_keys = (uint8_t *) malloc(NUMBER_OF_COLUMNS * (NUMBER_OF_ROUNDS + 1) * NUMBER_OF_ROWS);
    if (round_keys == NULL) {
        printf("malloc failed\n");
        //TODO handle this
        return NULL;
    }
    expand_key(key, round_keys);
#if USE_TBOX_IMPL == 1
    compute_t_boxes();
#endif

    return round_keys;
}

inline void ShiftRows(uint8_t *state) {
    uint8_t temp;

    // Row 1: shift left by 1
    temp = state[1];
    state[1] = state[5];
    state[5] = state[9];
    state[9] = state[13];
    state[13] = temp;

    // Row 2: shift left by 2
    temp = state[2];
    state[2] = state[10];
    state[10] = temp;
    temp = state[6];
    state[6] = state[14];
    state[14] = temp;

    // Row 3: shift left by 3 (equivalently right by 1)
    temp = state[3];
    state[3] = state[15];
    state[15] = state[11];
    state[11] = state[7];
    state[7] = temp;
}


inline void add_round_key(uint8_t *state, uint8_t *round_keys, uint8_t round) {
    // offset needed to get the correct column
    uint8_t offset = 16 * round;
    for (uint8_t i = 0; i < 16; i++) {
        state[i] ^= round_keys[offset + i];
    }
}


void print_block_two(const char *label, const uint8_t *block) {
    // printf("%s: ", label);
    // for (int i = 0; i < 16; i++) {
    //     printf("%02X ", block[i]);
    // }
    // printf("\n");
    }

void aes128_encrypt_classic(void *buffer, void *round_keys) {
    uint8_t *input_buffer = (uint8_t *) buffer;
    // buffer: the buffer which contains the cleartext/message which shall be encrypted
    uint8_t state[4 * NUMBER_OF_COLUMNS] = {0};
    // copy the input buffer into the state buffer (easier to debug)
    for (uint8_t i = 0; i < 4 * NUMBER_OF_COLUMNS; i++) {
        state[i] = input_buffer[i];
    }
    add_round_key(state, round_keys, 0); // first round

    for (uint8_t round = 1; round < NUMBER_OF_ROUNDS; round++) {
        SubBytes(state);
        ShiftRows(state);
        MixColumns(state);
        add_round_key(state, round_keys, round);
    }
    // last round
    SubBytes(state);
    ShiftRows(state);
    add_round_key(state, round_keys, NUMBER_OF_ROUNDS);
    // copy the state into the buffer again
    memcpy(input_buffer, state, 4 * NUMBER_OF_COLUMNS);
}



void aes128_encrypt(void *buffer, void *round_keys) {
#if USE_TBOX_IMPL == 1
    aes128_encrypt_tbox(buffer, round_keys);
#else
    aes128_encrypt_classic(buffer, round_keys);
#endif
}
