#include "nfc_util.h"

#include <furi.h>

static const uint8_t nfc_util_odd_byte_parity[256] = {
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0,
    1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1,
    1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1,
    0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1,
    0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0,
    0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1,
    0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1};

void nfc_util_num2bytes(uint64_t src, uint8_t len, uint8_t* dest) {
    furi_assert(dest);
    furi_assert(len <= 8);

    while(len--) {
        dest[len] = (uint8_t)src;
        src >>= 8;
    }
}

uint64_t nfc_util_bytes2num(uint8_t* src, uint8_t len) {
    furi_assert(src);
    furi_assert(len <= 8);

    uint64_t res = 0;
    while(len--) {
        res = (res << 8) | (*src);
        src++;
    }
    return res;
}

uint8_t nfc_util_even_parity32(uint32_t data) {
    // data ^= data >> 16;
    // data ^= data >> 8;
    // return !nfc_util_odd_byte_parity[data];
    return (__builtin_parity(data) & 0xFF);
}

uint8_t nfc_util_odd_parity8(uint8_t data) {
    return nfc_util_odd_byte_parity[data];
}

void nfc_util_get_parity(uint8_t* data, uint8_t len, uint8_t* par) {
    uint16_t paritybit_cnt = 0;
    uint16_t paritybyte_cnt = 0;
    uint8_t parityBits = 0;

    for(uint16_t i = 0; i < len; i++) {
        // Generate the parity bits
        parityBits |= ((nfc_util_odd_parity8(data[i])) << (7 - paritybit_cnt));
        if(paritybit_cnt == 7) {
            par[paritybyte_cnt] = parityBits; // save 8 Bits parity
            parityBits = 0; // and advance to next Parity Byte
            paritybyte_cnt++;
            paritybit_cnt = 0;
        } else {
            paritybit_cnt++;
        }
    }

    // save remaining parity bits
    par[paritybyte_cnt] = parityBits;
}
