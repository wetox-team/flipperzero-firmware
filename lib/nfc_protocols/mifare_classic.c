#include "mifare_classic.h"
#include <furi.h>
#include <furi-hal.h>

    bool
    mf_classic_check_card_type(uint8_t ATQA0, uint8_t ATQA1, uint8_t SAK) {
    if((ATQA0 == 0x42) && (ATQA1 == 0x00) && (SAK == 0x18)) {
        return true;
    } else if((ATQA0 == 0x44) && (ATQA1 == 0x00) && (SAK == 0x08)) {
        return true;
    } else if((ATQA0 == 0x04) && (ATQA1 == 0x00) && (SAK == 0x08)) {
        return true;
    }
    return false;
}

void mf_classic_set_default_version(MifareClassicDevice* mf_classic_read) {
    mf_classic_read->type = MfClassicTypeS50;
    mf_classic_read->blocks_to_read = 64;
    mf_classic_read->support_fast_read = false;
}

uint16_t mf_classic_prepare_read(uint8_t* dest, uint8_t start_block) {
    dest[0] = MF_CLASSIC_READ_CMD;
    dest[1] = start_block;
    return 2;
}

void mf_classic_parse_read_response(
    uint8_t* buff,
    uint16_t block_addr,
    MifareClassicDevice* mf_classic_read) {
    mf_classic_read->blocks_read += 1;
    mf_classic_read->data.data_size = mf_classic_read->blocks_read * 16;
    memcpy(&mf_classic_read->data.data[block_addr * 16], buff, 16);
}

uint16_t mf_classic_auth(uint8_t* dest, uint8_t block) {
    dest[0] = MF_CLASSIC_AUTH_A_CMD;
    dest[1] = block;
    return 2;
}


