#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "bucketsort.h"

typedef struct {
    uint32_t odd;
    uint32_t even;
} Crypto1;

void crypto1_reset(Crypto1* crypto1);

void crypto1_init(Crypto1* crypto1, uint64_t key);

uint8_t crypto1_bit(Crypto1* crypto1, uint8_t in, int is_encrypted);

uint8_t crypto1_byte(Crypto1* crypto1, uint8_t in, int is_encrypted);

uint32_t crypto1_word(Crypto1* crypto1, uint32_t in, int is_encrypted);

uint32_t crypto1_filter(uint32_t in);

uint32_t prng_successor(uint32_t x, uint32_t n);

void crypto1_get_lfsr(Crypto1*, uint64_t*);

Crypto1* lfsr_recovery32(uint32_t ks2, uint32_t in);
Crypto1* lfsr_recovery64(uint32_t ks2, uint32_t ks3);
Crypto1* lfsr_common_prefix(
    uint32_t pfx,
    uint32_t rr,
    uint8_t ks[8],
    uint8_t par[8][8],
    uint32_t no_par);
uint32_t* lfsr_prefix_ks(const uint8_t ks[8], int isodd);
int nonce_distance(uint32_t from, uint32_t to);
bool validate_prng_nonce(uint32_t nonce);
uint8_t lfsr_rollback_bit(Crypto1* s, uint32_t in, int fb);
uint8_t lfsr_rollback_byte(Crypto1* s, uint32_t in, int fb);
uint32_t lfsr_rollback_word(Crypto1* s, uint32_t in, int fb);