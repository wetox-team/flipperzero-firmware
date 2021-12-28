#include <stdbool.h>
#include <string.h>
#include "crypto1.h"

static uint16_t crc_table[256];
static bool crc_table_init = false;
static CrcType_t current_crc_type = CRC_NONE;
const uint8_t g_OddByteParity[256] = {
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};

uint8_t reflect8(uint8_t b) {
    return (b * 0x0202020202ULL & 0x010884422010ULL) % 1023;
}

int filter(uint32_t const x) {
    uint32_t f;

    f = 0xf22c0 >> (x & 0xf) & 16;
    f |= 0x6c9c0 >> (x >> 4 & 0xf) & 8;
    f |= 0x3c8b0 >> (x >> 8 & 0xf) & 4;
    f |= 0x1e458 >> (x >> 12 & 0xf) & 2;
    f |= 0x0d938 >> (x >> 16 & 0xf) & 1;
    return BIT(0xEC57E80A, f);
}

uint8_t crypto1_bit(struct Crypto1State* s, uint8_t in, int is_encrypted) {
    uint32_t feedin, t;
    uint8_t ret = filter(s->odd);

    feedin = ret & (!!is_encrypted);
    feedin ^= !!in;
    feedin ^= LF_POLY_ODD & s->odd;
    feedin ^= LF_POLY_EVEN & s->even;
    s->even = s->even << 1 | (evenparity32(feedin));

    t = s->odd;
    s->odd = s->even;
    s->even = t;

    return ret;
}

uint16_t reflect16(uint16_t b) {
    uint16_t v = 0;
    v |= (b & 0x8000) >> 15;
    v |= (b & 0x4000) >> 13;
    v |= (b & 0x2000) >> 11;
    v |= (b & 0x1000) >> 9;
    v |= (b & 0x0800) >> 7;
    v |= (b & 0x0400) >> 5;
    v |= (b & 0x0200) >> 3;
    v |= (b & 0x0100) >> 1;

    v |= (b & 0x0080) << 1;
    v |= (b & 0x0040) << 3;
    v |= (b & 0x0020) << 5;
    v |= (b & 0x0010) << 7;
    v |= (b & 0x0008) << 9;
    v |= (b & 0x0004) << 11;
    v |= (b & 0x0002) << 13;
    v |= (b & 0x0001) << 15;
    return v;
}

// table lookup LUT solution
uint16_t crc16_fast(uint8_t const* d, size_t n, uint16_t initval, bool refin, bool refout) {
    // fast lookup table algorithm without augmented zero bytes, e.g. used in pkzip.
    // only usable with polynom orders of 8, 16, 24 or 32.
    if(n == 0) return (~initval);

    uint16_t crc = initval;

    if(refin) crc = reflect16(crc);

    if(!refin)
        while(n--) crc = (crc << 8) ^ crc_table[((crc >> 8) ^ *d++) & 0xFF];
    else
        while(n--) crc = (crc >> 8) ^ crc_table[(crc & 0xFF) ^ *d++];

    if(refout ^ refin) crc = reflect16(crc);

    return crc;
}

// CRC-A (14443-3)
// poly=0x1021 init=0xc6c6 refin=true refout=true xorout=0x0000 name="CRC-A"
uint16_t crc16_a(uint8_t const* d, size_t n) {
    return crc16_fast(d, n, 0xC6C6, true, true);
}

void init_table(CrcType_t crctype) {
    // same crc algo, and initialised already
    if(crctype == current_crc_type && crc_table_init) return;

    // not the same crc algo. reset table.
    if(crctype != current_crc_type) reset_table();

    current_crc_type = crctype;

    switch(crctype) {
    case CRC_14443_A:
        generate_table(CRC16_POLY_CCITT, true);
        break;
    case CRC_NONE:
        crc_table_init = false;
        current_crc_type = CRC_NONE;
        break;
    }
}

void generate_table(uint16_t polynomial, bool refin) {
    for(uint16_t i = 0; i < 256; i++) {
        uint16_t c, crc = 0;
        if(refin)
            c = reflect8(i) << 8;
        else
            c = i << 8;

        for(uint16_t j = 0; j < 8; j++) {
            if((crc ^ c) & 0x8000)
                crc = (crc << 1) ^ polynomial;
            else
                crc = crc << 1;

            c = c << 1;
        }
        if(refin) crc = reflect16(crc);

        crc_table[i] = crc;
    }
    crc_table_init = true;
}

void reset_table(void) {
    memset(crc_table, 0, sizeof(crc_table));
    crc_table_init = false;
    current_crc_type = CRC_NONE;
}

uint32_t prng_successor(uint32_t x, uint32_t n) {
    SWAPENDIAN(x);
    while(n--) x = x >> 1 | (x >> 16 ^ x >> 18 ^ x >> 19 ^ x >> 21) << 31;

    return SWAPENDIAN(x);
}

uint8_t evenparity8(const uint8_t x) {
    return !g_OddByteParity[x];
}
uint8_t oddparity8(const uint8_t x) {
    return g_OddByteParity[x];
}

void num_to_bytes(uint64_t n, size_t len, uint8_t* dest) {
    while(len--) {
        dest[len] = (uint8_t)n;
        n >>= 8;
    }
}

void compute_crc(CrcType_t ct, const uint8_t* d, size_t n, uint8_t* first, uint8_t* second) {
    // can't calc a crc on less than 1 byte
    if(n == 0) return;

    init_table(ct);

    uint16_t crc = 0;
    switch(ct) {
    case CRC_14443_A:
        crc = crc16_a(d, n);
        break;
    case CRC_NONE:
        return;
    }
    *first = (crc & 0xFF);
    *second = ((crc >> 8) & 0xFF);
}

void crypto1_deinit(struct Crypto1State* state) {
    state->odd = 0;
    state->even = 0;
}

void crypto1_init(struct Crypto1State* state, uint64_t key) {
    if(state == NULL) return;
    state->odd = 0;
    state->even = 0;
    for(int i = 47; i > 0; i -= 2) {
        state->odd = state->odd << 1 | BIT(key, (i - 1) ^ 7);
        state->even = state->even << 1 | BIT(key, i ^ 7);
    }
}

uint8_t crypto1_byte(struct Crypto1State* s, uint8_t in, int is_encrypted) {
    uint8_t ret = 0;
    ret |= crypto1_bit(s, BIT(in, 0), is_encrypted) << 0;
    ret |= crypto1_bit(s, BIT(in, 1), is_encrypted) << 1;
    ret |= crypto1_bit(s, BIT(in, 2), is_encrypted) << 2;
    ret |= crypto1_bit(s, BIT(in, 3), is_encrypted) << 3;
    ret |= crypto1_bit(s, BIT(in, 4), is_encrypted) << 4;
    ret |= crypto1_bit(s, BIT(in, 5), is_encrypted) << 5;
    ret |= crypto1_bit(s, BIT(in, 6), is_encrypted) << 6;
    ret |= crypto1_bit(s, BIT(in, 7), is_encrypted) << 7;
    return ret;
}
uint32_t crypto1_word(struct Crypto1State* s, uint32_t in, int is_encrypted) {
    uint32_t ret = 0;
    // note: xor args have been swapped because some compilers emit a warning
    // for 10^x and 2^x as possible misuses for exponentiation. No comment.
    ret |= crypto1_bit(s, BEBIT(in, 0), is_encrypted) << (24 ^ 0);
    ret |= crypto1_bit(s, BEBIT(in, 1), is_encrypted) << (24 ^ 1);
    ret |= crypto1_bit(s, BEBIT(in, 2), is_encrypted) << (24 ^ 2);
    ret |= crypto1_bit(s, BEBIT(in, 3), is_encrypted) << (24 ^ 3);
    ret |= crypto1_bit(s, BEBIT(in, 4), is_encrypted) << (24 ^ 4);
    ret |= crypto1_bit(s, BEBIT(in, 5), is_encrypted) << (24 ^ 5);
    ret |= crypto1_bit(s, BEBIT(in, 6), is_encrypted) << (24 ^ 6);
    ret |= crypto1_bit(s, BEBIT(in, 7), is_encrypted) << (24 ^ 7);

    ret |= crypto1_bit(s, BEBIT(in, 8), is_encrypted) << (24 ^ 8);
    ret |= crypto1_bit(s, BEBIT(in, 9), is_encrypted) << (24 ^ 9);
    ret |= crypto1_bit(s, BEBIT(in, 10), is_encrypted) << (24 ^ 10);
    ret |= crypto1_bit(s, BEBIT(in, 11), is_encrypted) << (24 ^ 11);
    ret |= crypto1_bit(s, BEBIT(in, 12), is_encrypted) << (24 ^ 12);
    ret |= crypto1_bit(s, BEBIT(in, 13), is_encrypted) << (24 ^ 13);
    ret |= crypto1_bit(s, BEBIT(in, 14), is_encrypted) << (24 ^ 14);
    ret |= crypto1_bit(s, BEBIT(in, 15), is_encrypted) << (24 ^ 15);

    ret |= crypto1_bit(s, BEBIT(in, 16), is_encrypted) << (24 ^ 16);
    ret |= crypto1_bit(s, BEBIT(in, 17), is_encrypted) << (24 ^ 17);
    ret |= crypto1_bit(s, BEBIT(in, 18), is_encrypted) << (24 ^ 18);
    ret |= crypto1_bit(s, BEBIT(in, 19), is_encrypted) << (24 ^ 19);
    ret |= crypto1_bit(s, BEBIT(in, 20), is_encrypted) << (24 ^ 20);
    ret |= crypto1_bit(s, BEBIT(in, 21), is_encrypted) << (24 ^ 21);
    ret |= crypto1_bit(s, BEBIT(in, 22), is_encrypted) << (24 ^ 22);
    ret |= crypto1_bit(s, BEBIT(in, 23), is_encrypted) << (24 ^ 23);

    ret |= crypto1_bit(s, BEBIT(in, 24), is_encrypted) << (24 ^ 24);
    ret |= crypto1_bit(s, BEBIT(in, 25), is_encrypted) << (24 ^ 25);
    ret |= crypto1_bit(s, BEBIT(in, 26), is_encrypted) << (24 ^ 26);
    ret |= crypto1_bit(s, BEBIT(in, 27), is_encrypted) << (24 ^ 27);
    ret |= crypto1_bit(s, BEBIT(in, 28), is_encrypted) << (24 ^ 28);
    ret |= crypto1_bit(s, BEBIT(in, 29), is_encrypted) << (24 ^ 29);
    ret |= crypto1_bit(s, BEBIT(in, 30), is_encrypted) << (24 ^ 30);
    ret |= crypto1_bit(s, BEBIT(in, 31), is_encrypted) << (24 ^ 31);
    return ret;
}