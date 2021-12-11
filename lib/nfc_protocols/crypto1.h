#pragma once

#include <stdlib.h>
#include <stdint.h>

#define SWAPENDIAN(x) (x = (x >> 8 & 0xff00ff) | (x & 0xff00ff) << 8, x = x >> 16 | x << 16)
#define BIT(x, n) ((x) >> (n)&1)
#define BEBIT(x, n) BIT(x, (n) ^ 24)
#define CRC16_POLY_CCITT 0x1021

static inline int filter(uint32_t const x) {
    uint32_t f;

    f = 0xf22c0 >> (x & 0xf) & 16;
    f |= 0x6c9c0 >> (x >> 4 & 0xf) & 8;
    f |= 0x3c8b0 >> (x >> 8 & 0xf) & 4;
    f |= 0x1e458 >> (x >> 12 & 0xf) & 2;
    f |= 0x0d938 >> (x >> 16 & 0xf) & 1;
    return BIT(0xEC57E80A, f);
}

#define LF_POLY_ODD (0x29CE5C)
#define LF_POLY_EVEN (0x870804)

struct Crypto1State {uint32_t odd, even;};

void crypto1_init(struct Crypto1State* state, uint64_t key);
void crypto1_deinit(struct Crypto1State*);

static inline uint8_t oddparity16(uint16_t x) {
  return !__builtin_parity(x);
}

static inline uint8_t evenparity32(uint32_t x) {
  return (__builtin_parity(x) & 0xFF);
}

typedef enum {
    CRC_NONE,
    CRC_14443_A,
} CrcType_t;

uint32_t prng_successor(uint32_t x, uint32_t n);
uint32_t crypto1_word(struct Crypto1State* s, uint32_t in, int is_encrypted);
void compute_crc(CrcType_t ct, const uint8_t* d, size_t n, uint8_t* first, uint8_t* second);
uint8_t crypto1_byte(struct Crypto1State* s, uint8_t in, int is_encrypted);
void num_to_bytes(uint64_t n, size_t len, uint8_t* dest);
uint8_t crypto1_bit(struct Crypto1State* s, uint8_t in, int is_encrypted);

extern const uint8_t g_OddByteParity[256];

uint8_t evenparity8(const uint8_t x);
uint8_t oddparity8(const uint8_t x);

// table implementation

void init_table(CrcType_t crctype);
void reset_table(void);
void generate_table(uint16_t polynomial, bool refin);