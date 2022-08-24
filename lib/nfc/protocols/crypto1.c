#include "crypto1.h"
#include "nfc_util.h"
#include <furi.h>

// Algorithm from https://github.com/RfidResearchGroup/proxmark3.git

#define SWAPENDIAN(x) (x = (x >> 8 & 0xff00ff) | (x & 0xff00ff) << 8, x = x >> 16 | x << 16)
#define LF_POLY_ODD (0x29CE5C)
#define LF_POLY_EVEN (0x870804)

#define BEBIT(x, n) FURI_BIT(x, (n) ^ 24)
#define BIT(x, n) ((x) >> (n)&1)

void crypto1_reset(Crypto1* crypto1) {
    furi_assert(crypto1);
    crypto1->even = 0;
    crypto1->odd = 0;
}

void crypto1_init(Crypto1* crypto1, uint64_t key) {
    furi_assert(crypto1);
    crypto1->even = 0;
    crypto1->odd = 0;
    for(int8_t i = 47; i > 0; i -= 2) {
        crypto1->odd = crypto1->odd << 1 | FURI_BIT(key, (i - 1) ^ 7);
        crypto1->even = crypto1->even << 1 | FURI_BIT(key, i ^ 7);
    }
}

uint32_t crypto1_filter(uint32_t in) {
    uint32_t out = 0;
    out = 0xf22c0 >> (in & 0xf) & 16;
    out |= 0x6c9c0 >> (in >> 4 & 0xf) & 8;
    out |= 0x3c8b0 >> (in >> 8 & 0xf) & 4;
    out |= 0x1e458 >> (in >> 12 & 0xf) & 2;
    out |= 0x0d938 >> (in >> 16 & 0xf) & 1;
    return FURI_BIT(0xEC57E80A, out);
}

uint8_t crypto1_bit(Crypto1* crypto1, uint8_t in, int is_encrypted) {
    furi_assert(crypto1);
    uint8_t out = crypto1_filter(crypto1->odd);
    uint32_t feed = out & (!!is_encrypted);
    feed ^= !!in;
    feed ^= LF_POLY_ODD & crypto1->odd;
    feed ^= LF_POLY_EVEN & crypto1->even;
    crypto1->even = crypto1->even << 1 | (nfc_util_even_parity32(feed));

    FURI_SWAP(crypto1->odd, crypto1->even);
    return out;
}

uint8_t crypto1_byte(Crypto1* crypto1, uint8_t in, int is_encrypted) {
    furi_assert(crypto1);
    uint8_t out = 0;
    for(uint8_t i = 0; i < 8; i++) {
        out |= crypto1_bit(crypto1, FURI_BIT(in, i), is_encrypted) << i;
    }
    return out;
}

uint32_t crypto1_word(Crypto1* crypto1, uint32_t in, int is_encrypted) {
    furi_assert(crypto1);
    uint32_t out = 0;
    for(uint8_t i = 0; i < 32; i++) {
        out |= crypto1_bit(crypto1, BEBIT(in, i), is_encrypted) << (24 ^ i);
    }
    return out;
}

uint32_t prng_successor(uint32_t x, uint32_t n) {
    SWAPENDIAN(x);
    while(n--) x = x >> 1 | (x >> 16 ^ x >> 18 ^ x >> 19 ^ x >> 21) << 31;

    return SWAPENDIAN(x);
}

void crypto1_get_lfsr(Crypto1* state, uint64_t* lfsr) {
    int i;
    for(*lfsr = 0, i = 23; i >= 0; --i) {
        *lfsr = *lfsr << 1 | BIT(state->odd, i ^ 3);
        *lfsr = *lfsr << 1 | BIT(state->even, i ^ 3);
    }
}

/** update_contribution
 * helper, calculates the partial linear feedback contributions and puts in MSB
 */
static inline void
    update_contribution(uint32_t* item, const uint32_t mask1, const uint32_t mask2) {
    uint32_t p = *item >> 25;

    p = p << 1 | (nfc_util_even_parity32(*item & mask1));
    p = p << 1 | (nfc_util_even_parity32(*item & mask2));
    *item = p << 24 | (*item & 0xffffff);
}

/** extend_table
 * using a bit of the keystream extend the table of possible lfsr states
 */
static inline void
    extend_table(uint32_t* tbl, uint32_t** end, uint32_t bit, int m1, int m2, uint32_t in) {
    in <<= 24;
    for(*tbl <<= 1; tbl <= *end; *++tbl <<= 1)
        if(crypto1_filter(*tbl) ^ crypto1_filter(*tbl | 1)) {
            *tbl |= crypto1_filter(*tbl) ^ bit;
            update_contribution(tbl, m1, m2);
            *tbl ^= in;
        } else if(crypto1_filter(*tbl) == bit) {
            *++*end = tbl[1];
            tbl[1] = tbl[0] | 1;
            update_contribution(tbl, m1, m2);
            *tbl++ ^= in;
            update_contribution(tbl, m1, m2);
            *tbl ^= in;
        } else
            *tbl-- = *(*end)--;
}
/** extend_table_simple
 * using a bit of the keystream extend the table of possible lfsr states
 */
static inline void extend_table_simple(uint32_t* tbl, uint32_t** end, uint32_t bit) {
    for(*tbl <<= 1; tbl <= *end; *++tbl <<= 1) {
        if(crypto1_filter(*tbl) ^ crypto1_filter(*tbl | 1)) { // replace
            *tbl |= crypto1_filter(*tbl) ^ bit;
        } else if(crypto1_filter(*tbl) == bit) { // insert
            *++*end = *++tbl;
            *tbl = tbl[-1] | 1;
        } else { // drop
            *tbl-- = *(*end)--;
        }
    }
}

/** recover
 * recursively narrow down the search space, 4 bits of keystream at a time
 */
static Crypto1* recover(
    uint32_t* o_head,
    uint32_t* o_tail,
    uint32_t oks,
    uint32_t* e_head,
    uint32_t* e_tail,
    uint32_t eks,
    int rem,
    Crypto1* sl,
    uint32_t in,
    bucket_array_t bucket) {
    bucket_info_t bucket_info;

    if(rem == -1) {
        for(uint32_t* e = e_head; e <= e_tail; ++e) {
            *e = *e << 1 ^ (nfc_util_even_parity32(*e & LF_POLY_EVEN)) ^ (!!(in & 4));
            for(uint32_t* o = o_head; o <= o_tail; ++o, ++sl) {
                sl->even = *o;
                sl->odd = *e ^ (nfc_util_even_parity32(*o & LF_POLY_ODD));
                sl[1].odd = sl[1].even = 0;
            }
        }
        return sl;
    }

    for(uint32_t i = 0; i < 4 && rem--; i++) {
        oks >>= 1;
        eks >>= 1;
        in >>= 2;
        extend_table(o_head, &o_tail, oks & 1, LF_POLY_EVEN << 1 | 1, LF_POLY_ODD << 1, 0);
        if(o_head > o_tail) return sl;

        extend_table(e_head, &e_tail, eks & 1, LF_POLY_ODD, LF_POLY_EVEN << 1 | 1, in & 3);
        if(e_head > e_tail) return sl;
    }

    bucket_sort_intersect(e_head, e_tail, o_head, o_tail, &bucket_info, bucket);

    for(int i = bucket_info.numbuckets - 1; i >= 0; i--) {
        sl = recover(
            bucket_info.bucket_info[1][i].head,
            bucket_info.bucket_info[1][i].tail,
            oks,
            bucket_info.bucket_info[0][i].head,
            bucket_info.bucket_info[0][i].tail,
            eks,
            rem,
            sl,
            in,
            bucket);
    }

    return sl;
}

/** lfsr_recovery
 * recover the state of the lfsr given 32 bits of the keystream
 * additionally you can use the in parameter to specify the value
 * that was fed into the lfsr at the time the keystream was generated
 */
Crypto1* lfsr_recovery32(uint32_t ks2, uint32_t in) {
    Crypto1* statelist;
    uint32_t *odd_head = 0, *odd_tail = 0, oks = 0;
    uint32_t *even_head = 0, *even_tail = 0, eks = 0;
    int i;

    // split the keystream into an odd and even part
    for(i = 31; i >= 0; i -= 2) oks = oks << 1 | BEBIT(ks2, i);
    for(i = 30; i >= 0; i -= 2) eks = eks << 1 | BEBIT(ks2, i);

    odd_head = odd_tail = malloc(sizeof(uint32_t) << 21);
    memset(odd_head, 0, sizeof(uint32_t) << 21);
    memset(odd_tail, 0, sizeof(uint32_t) << 21);
    even_head = even_tail = malloc(sizeof(uint32_t) << 21);
    memset(even_head, 0, sizeof(uint32_t) << 21);
    memset(even_tail, 0, sizeof(uint32_t) << 21);
    statelist = malloc(sizeof(Crypto1) << 18);
    memset(statelist, 1, sizeof(Crypto1) << 18);

    // allocate memory for out of place bucket_sort
    bucket_array_t bucket;

    for(i = 0; i < 2; i++) {
        for(uint32_t j = 0; j <= 0xff; j++) {
            bucket[i][j].head = malloc(sizeof(uint32_t) << 14);
            memset(bucket[i][j].head, 0, sizeof(uint32_t) << 14);
            if(!bucket[i][j].head) {
                goto out;
            }
        }
    }

    // initialize statelists: add all possible states which would result into the rightmost 2 bits of the keystream
    for(i = 1 << 20; i >= 0; --i) {
        if(crypto1_filter(i) == (oks & 1)) *++odd_tail = i;
        if(crypto1_filter(i) == (eks & 1)) *++even_tail = i;
    }

    // extend the statelists. Look at the next 8 Bits of the keystream (4 Bit each odd and even):
    for(i = 0; i < 4; i++) {
        extend_table_simple(odd_head, &odd_tail, (oks >>= 1) & 1);
        extend_table_simple(even_head, &even_tail, (eks >>= 1) & 1);
    }

    // the statelists now contain all states which could have generated the last 10 Bits of the keystream.
    // 22 bits to go to recover 32 bits in total. From now on, we need to take the "in"
    // parameter into account.
    in = (in >> 16 & 0xff) | (in << 16) | (in & 0xff00); // Byte swapping
    recover(odd_head, odd_tail, oks, even_head, even_tail, eks, 11, statelist, in << 1, bucket);

out:
    for(i = 0; i < 2; i++)
        for(uint32_t j = 0; j <= 0xff; j++) free(bucket[i][j].head);
    free(odd_head);
    free(even_head);
    return statelist;
}

/** lfsr_rollback_bit
 * Rollback the shift register in order to get previous states
 */
uint8_t lfsr_rollback_bit(Crypto1* s, uint32_t in, int fb) {
    int out;
    uint8_t ret;
    uint32_t t;

    s->odd &= 0xffffff;
    t = s->odd, s->odd = s->even, s->even = t;

    out = s->even & 1;
    out ^= LF_POLY_EVEN & (s->even >>= 1);
    out ^= LF_POLY_ODD & s->odd;
    out ^= !!in;
    out ^= (ret = crypto1_filter(s->odd)) & (!!fb);

    s->even |= (nfc_util_even_parity32(out)) << 23;
    return ret;
}
/** lfsr_rollback_byte
 * Rollback the shift register in order to get previous states
 */
uint8_t lfsr_rollback_byte(Crypto1* s, uint32_t in, int fb) {
    uint8_t ret = 0;
    ret |= lfsr_rollback_bit(s, BIT(in, 7), fb) << 7;
    ret |= lfsr_rollback_bit(s, BIT(in, 6), fb) << 6;
    ret |= lfsr_rollback_bit(s, BIT(in, 5), fb) << 5;
    ret |= lfsr_rollback_bit(s, BIT(in, 4), fb) << 4;
    ret |= lfsr_rollback_bit(s, BIT(in, 3), fb) << 3;
    ret |= lfsr_rollback_bit(s, BIT(in, 2), fb) << 2;
    ret |= lfsr_rollback_bit(s, BIT(in, 1), fb) << 1;
    ret |= lfsr_rollback_bit(s, BIT(in, 0), fb) << 0;
    return ret;
}
/** lfsr_rollback_word
 * Rollback the shift register in order to get previous states
 */
uint32_t lfsr_rollback_word(Crypto1* s, uint32_t in, int fb) {
    uint32_t ret = 0;
    // note: xor args have been swapped because some compilers emit a warning
    // for 10^x and 2^x as possible misuses for exponentiation. No comment.
    ret |= lfsr_rollback_bit(s, BEBIT(in, 31), fb) << (24 ^ 31);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 30), fb) << (24 ^ 30);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 29), fb) << (24 ^ 29);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 28), fb) << (24 ^ 28);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 27), fb) << (24 ^ 27);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 26), fb) << (24 ^ 26);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 25), fb) << (24 ^ 25);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 24), fb) << (24 ^ 24);

    ret |= lfsr_rollback_bit(s, BEBIT(in, 23), fb) << (24 ^ 23);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 22), fb) << (24 ^ 22);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 21), fb) << (24 ^ 21);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 20), fb) << (24 ^ 20);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 19), fb) << (24 ^ 19);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 18), fb) << (24 ^ 18);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 17), fb) << (24 ^ 17);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 16), fb) << (24 ^ 16);

    ret |= lfsr_rollback_bit(s, BEBIT(in, 15), fb) << (24 ^ 15);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 14), fb) << (24 ^ 14);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 13), fb) << (24 ^ 13);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 12), fb) << (24 ^ 12);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 11), fb) << (24 ^ 11);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 10), fb) << (24 ^ 10);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 9), fb) << (24 ^ 9);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 8), fb) << (24 ^ 8);

    ret |= lfsr_rollback_bit(s, BEBIT(in, 7), fb) << (24 ^ 7);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 6), fb) << (24 ^ 6);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 5), fb) << (24 ^ 5);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 4), fb) << (24 ^ 4);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 3), fb) << (24 ^ 3);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 2), fb) << (24 ^ 2);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 1), fb) << (24 ^ 1);
    ret |= lfsr_rollback_bit(s, BEBIT(in, 0), fb) << (24 ^ 0);
    return ret;
}

/** nonce_distance
 * x,y valid tag nonces, then prng_successor(x, nonce_distance(x, y)) = y
 */
static uint16_t* dist = 0;
int nonce_distance(uint32_t from, uint32_t to) {
    if(!dist) {
        // allocation 2bytes * 0xFFFF times.
        dist = malloc(2 << 16 * sizeof(uint8_t));
        memset(dist, 0, 2 << 16 * sizeof(uint8_t));
        if(!dist) return -1;
        uint16_t x = 1;
        for(uint16_t i = 1; i; ++i) {
            dist[(x & 0xff) << 8 | x >> 8] = i;
            x = x >> 1 | (x ^ x >> 2 ^ x >> 3 ^ x >> 5) << 15;
        }
    }
    return (65535 + dist[to >> 16] - dist[from >> 16]) % 65535;
}

/** validate_prng_nonce
 * Determine if nonce is deterministic. ie: Suspectable to Darkside attack.
 * returns
 *   true = weak prng
 *   false = hardend prng
 */
bool validate_prng_nonce(uint32_t nonce) {
    // init prng table:
    if(nonce_distance(nonce, nonce) == -1) return false;
    return ((65535 - dist[nonce >> 16] + dist[nonce & 0xffff]) % 65535) == 16;
}
