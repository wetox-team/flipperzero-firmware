#include "mifare_classic.h"
#include <furi.h>
#include <furi-hal.h>

#define AddCrc14A(data, len) compute_crc(CRC_14443_A, (data), (len), (data) + (len), (data) + (len) + 1)

bool mf_classic_check_card_type(uint8_t ATQA0, uint8_t ATQA1, uint8_t SAK) {
    if((ATQA0 == 0x42) && (ATQA1 == 0x00) && (SAK == 0x18)) {
        return true;
    } else if((ATQA0 == 0x44) && (ATQA1 == 0x00) && (SAK == 0x08)) {
        return true;
    } else if((ATQA0 == 0x04) && (ATQA1 == 0x00) && (SAK == 0x08)) {
        return true;
    }
    return false;
}

uint64_t bytes_to_num(uint8_t* src, size_t len) {
    uint64_t num = 0;
    while(len--) {
        num = (num << 8) | (*src);
        src++;
    }
    return num;
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

// send 2 byte commands
int mifare_sendcmd_short(
    struct Crypto1State* pcs,
    uint8_t crypted,
    uint8_t cmd,
    uint8_t data,
    uint8_t* answer,
    uint8_t* answer_parity,
    uint32_t* timing) {
    uint16_t pos;
    uint8_t dcmd[2] = {cmd, data};
    uint8_t ecmd[2] = {0x00, 0x00};
    uint16_t* rx_len;
    uint8_t par[1] = {0x00}; // 1 Byte parity is enough here
    AddCrc14A(dcmd, 2);
    memcpy(ecmd, dcmd, sizeof(dcmd));
    FURI_LOG_I("ASTRA", "rx_len %d", &rx_len);
    if(pcs && crypted) {
        FURI_LOG_I("ASTRA", "crypted %d", crypted);
        par[0] = 0;
        for(pos = 0; pos < 2; pos++) {
            ecmd[pos] = crypto1_byte(pcs, 0x00, 0) ^ dcmd[pos];
            par[0] |= (((filter(pcs->odd) ^ oddparity8(dcmd[pos])) & 0x01) << (7 - pos));
        }
        // ReaderTransmitPar(ecmd, sizeof(ecmd), par, timing);
        furi_hal_nfc_data_exchange(ecmd, sizeof(ecmd), &answer, &rx_len, false);
    } else {
        // ReaderTransmit(dcmd, sizeof(dcmd), timing);
        furi_hal_nfc_data_exchange(dcmd, sizeof(dcmd), &answer, &rx_len, false);
    }

    //int len = ReaderReceive(answer, par);
    FURI_LOG_I("ASTRA", "LEN %d", &rx_len);
    int len = sizeof(&answer);
    FURI_LOG_I("ASTRA", "len %d", len);

    if(answer_parity) *answer_parity = par[0];

    if(crypted == CRYPT_ALL) {
        FURI_LOG_I("ASTRA", "crypted");
        if(len == 1) {
            uint16_t res = 0;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 0)) << 0;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 1)) << 1;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 2)) << 2;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 3)) << 3;
            answer[0] = res;
        } else {
            for(pos = 0; pos < len; pos++) answer[pos] = crypto1_byte(pcs, 0x00, 0) ^ answer[pos];
        }
    }
    return len;
}

int mifare_classic_auth(
    struct Crypto1State* pcs,
    uint32_t uid,
    uint8_t blockNo,
    uint8_t keyType,
    uint64_t ui64Key,
    uint8_t isNested,
    uint8_t* rx_buff,
    uint16_t* rx_len) {
    return mifare_classic_authex(pcs, uid, blockNo, keyType, ui64Key, isNested, rx_buff, rx_len, NULL, NULL);
}

int mifare_classic_authex(
    struct Crypto1State* pcs,
    uint32_t uid,
    uint8_t blockNo,
    uint8_t keyType,
    uint64_t ui64Key,
    uint8_t isNested,
    uint8_t* rx_buff,
    uint16_t* rx_len,
    uint32_t* ntptr,
    uint32_t* timing) {
    int len;
    uint32_t pos, nt, ntpp; // Supplied tag nonce
    uint8_t par[1] = {0x00};
    uint8_t nr[4];
    uint8_t mf_nr_ar[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t receivedAnswer[MAX_MIFARE_FRAME_SIZE] = {0x00};
    uint8_t receivedAnswerPar[MAX_MIFARE_PARITY_SIZE] = {0x00};

    // "random" reader nonce:
    num_to_bytes(prng_successor(DWT->CYCCNT, 32), 4, nr);

    // Transmit MIFARE_CLASSIC_AUTH
    len = mifare_sendcmd_short(
        pcs, isNested, 0x60 + (keyType & 0x01), blockNo, receivedAnswer, receivedAnswerPar, timing);
    FURI_LOG_I("ASTRA", "%d", len);
    if(len != 4) return 1;

    // Save the tag nonce (nt)
    nt = bytes_to_num(receivedAnswer, 4);

    //  ----------------------------- crypto1 create
    if(isNested) crypto1_deinit(pcs);

    // Init cipher with key
    crypto1_init(pcs, ui64Key);

    if(isNested == AUTH_NESTED) {
        // decrypt nt with help of new key
        nt = crypto1_word(pcs, nt ^ uid, 1) ^ nt;
    } else {
        // Load (plain) uid^nt into the cipher
        crypto1_word(pcs, nt ^ uid, 0);
    }

    // save Nt
    if(ntptr) *ntptr = nt;

    // Generate (encrypted) nr+parity by loading it into the cipher (Nr)
    par[0] = 0;
    for(pos = 0; pos < 4; pos++) {
        mf_nr_ar[pos] = crypto1_byte(pcs, nr[pos], 0) ^ nr[pos];
        par[0] |= (((filter(pcs->odd) ^ oddparity8(nr[pos])) & 0x01) << (7 - pos));
    }

    // Skip 32 bits in pseudo random generator
    nt = prng_successor(nt, 32);

    //  ar+parity
    for(pos = 4; pos < 8; pos++) {
        nt = prng_successor(nt, 8);
        mf_nr_ar[pos] = crypto1_byte(pcs, 0x00, 0) ^ (nt & 0xff);
        par[0] |= (((filter(pcs->odd) ^ oddparity8(nt & 0xff)) & 0x01) << (7 - pos));
    }

    // Transmit reader nonce and reader answer
    //ReaderTransmitPar(mf_nr_ar, sizeof(mf_nr_ar), par, NULL);
    FURI_LOG_I("ASTRA", "ReaderTransmit %08X", &mf_nr_ar);
    FURI_LOG_I("ASTRA", "Size %d", sizeof(mf_nr_ar));
    furi_hal_nfc_data_exchange(mf_nr_ar, sizeof(mf_nr_ar), &rx_buff, &rx_len, 0);
    FURI_LOG_I("ASTRA", "ReaderTransmit done");
    //

    // Receive 4 byte tag answer
    //len = ReaderReceive(receivedAnswer, receivedAnswerPar);
    len = (int)&rx_len;

    if(!len) {
        return 2;
    }

    ntpp = prng_successor(nt, 32) ^ crypto1_word(pcs, 0, 0);

    if(ntpp != bytes_to_num(rx_buff, 4)) {
        return 3;
    }
    return 0;
}