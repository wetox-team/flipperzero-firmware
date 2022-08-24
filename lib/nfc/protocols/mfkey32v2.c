#define TAG "mfkey32v2"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "crypto1.h"
#include "furi.h"

#define BIT(x, n) ((x) >> (n)&1)

uint32_t mfkey32v2(
    uint32_t uid,
    uint32_t nt0,
    uint32_t nt1,
    uint32_t nr0_enc,
    uint32_t ar0_enc,
    uint32_t nr1_enc,
    uint32_t ar1_enc) {
    Crypto1 *s, *t;
    uint64_t key;
    uint32_t ks2;

    FURI_LOG_I(TAG, "MfKey32v2 open source Mifare Classic key-recovery tool\n");
    FURI_LOG_I(TAG, "Cracks keys by two 32bit keystream authentications");

    // Generate lfsr successors of the tag challenge
    FURI_LOG_I(TAG, "\nLFSR successors of the tag challenge:\n");
    uint32_t p64 = prng_successor(nt0, 64);
    uint32_t p64b = prng_successor(nt1, 64);

    FURI_LOG_I(TAG, "  nt': %08x\n", p64);
    FURI_LOG_I(TAG, " nt'': %08x\n", prng_successor(p64, 32));

    // Extract the keystream from the messages
    FURI_LOG_I(TAG, "\nKeystream used to generate {ar} and {at}:\n");
    ks2 = ar0_enc ^ p64;
    FURI_LOG_I(TAG, "  ks2: %08x\n", ks2);

    s = lfsr_recovery32(ar0_enc ^ p64, 0);

    for(t = s; t->odd | t->even; ++t) {
        lfsr_rollback_word(t, 0, 0);
        lfsr_rollback_word(t, nr0_enc, 1);
        lfsr_rollback_word(t, uid ^ nt0, 0);
        crypto1_get_lfsr(t, &key);

        crypto1_word(t, uid ^ nt1, 0);
        crypto1_word(t, nr1_enc, 1);
        if(ar1_enc == (crypto1_word(t, 0, 0) ^ p64b)) {
            FURI_LOG_W(TAG, "\nFound Key: [%012" PRIx64 "]\n\n", key);
            break;
        }
    }
    free(s);
    return 0;
}