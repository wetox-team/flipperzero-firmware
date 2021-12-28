#include "mifare_classic.h"
#include <furi.h>
#include <furi-hal.h>
#include <stdlib.h>

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

uint64_t bytes_to_num(uint8_t *src, size_t len) {
    FURI_LOG_I("BTN", "src: %p len: %d", &src, len);
    uint64_t num = 0;
    while (len--) {
        num = (num << 8) | (*src);
        src++;
        FURI_LOG_I("BTN", "src: %p, len: %d, num: %d", src, len, num);
    }
    FURI_LOG_I("BTN", "returning number: %d", num);
    return num;
}

void mf_classic_set_default_version(MifareClassicDevice* mf_classic_read) {
    mf_classic_read->type = MfClassicTypeS50;
    mf_classic_read->blocks_to_read = 64;
    mf_classic_read->support_fast_read = false;
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
    uint32_t* timing) {
    uint16_t pos;
    uint16_t* len;
    uint8_t dcmd[2] = {cmd, data};
    uint8_t ecmd[2] = {0x00, 0x00};
    AddCrc14A(dcmd, 2);
    memcpy(ecmd, dcmd, sizeof(dcmd));
    uint8_t* answer2 = answer;
    if(pcs && crypted) {
        for(pos = 0; pos < 2; pos++) {
            ecmd[pos] = crypto1_byte(pcs, 0x00, 0) ^ dcmd[pos];
        }
        //ReaderTransmitPar(ecmd, sizeof(ecmd), par, timing);
        FURI_LOG_I("ASTRA", "ecmd = %02X", ecmd[1]);
        furi_hal_nfc_data_exchange(ecmd, sizeof(ecmd), &answer2, &len, false);
    } else {
        //ReaderTransmit(dcmd, sizeof(dcmd), timing);
        FURI_LOG_I("ASTRA", "dcmd = %02X", dcmd[1]);
        furi_hal_nfc_data_exchange(dcmd, sizeof(dcmd), &answer2, &len, false);
    }

    memcpy(answer, answer2, *len);

    //int len = ReaderReceive(answer, par);

    if(crypted == CRYPT_ALL) {
        if(*len == 1) {
            FURI_LOG_I("ASTRA", "len = 1");
            uint16_t res = 0;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 0)) << 0;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 1)) << 1;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 2)) << 2;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 3)) << 3;
            answer[0] = res;
        } else {
            FURI_LOG_I("ASTRA", "len = %d", *len);
            for(pos = 0; pos < *len; pos++) answer[pos] = crypto1_byte(pcs, 0x00, 0) ^ answer[pos];
        }
    }
    return *len;
}

// send 2 byte commands without crc
int mifare_sendcmd_short_no_crc(
    struct Crypto1State* pcs,
    uint8_t crypted,
    uint8_t cmd,
    uint8_t data,
    uint8_t* answer) {
    uint16_t pos;
    uint8_t* rx_parbits;
    uint16_t* len;
    uint8_t dcmd[4] = {cmd, data, 0x00, 0x00};
    uint8_t ecmd[4] = {0x00, 0x00, 0x00, 0x00};
    uint8_t par[1] = {0x00}; // 1 Byte parity is enough here
    AddCrc14A(dcmd, 2);
    memcpy(ecmd, dcmd, sizeof(dcmd));
    uint8_t* answer2 = answer;

    if(pcs && crypted) {
        par[0] = 0;
        for(pos = 0; pos < 4; pos++) {
            ecmd[pos] = crypto1_byte(pcs, 0x00, 0) ^ dcmd[pos];
            par[0] |= (((filter(pcs->odd) ^ oddparity8(dcmd[pos])) & 0x01) << (7 - pos));
        }
        //ReaderTransmitPar(ecmd, sizeof(ecmd), par, timing);
        FURI_LOG_I("MIFARE", "ecmd = %02X", ecmd[1]);
        FURI_LOG_I("MIFARE", "par = %02X", par[0]);
        furi_hal_nfc_raw_parbits_exchange(
            ecmd, sizeof(ecmd), par, &answer2, &len, &rx_parbits, false);
    } else {
        //ReaderTransmit(dcmd, sizeof(dcmd), timing);
        FURI_LOG_I("MIFARE", "dcmd = %02X", dcmd[1]);
        FURI_LOG_I("MIFARE", "par = %02X", par[0]);
        furi_hal_nfc_raw_parbits_exchange(
            dcmd, sizeof(dcmd), par, &answer2, &len, &rx_parbits, false);
    }

    memcpy(answer, answer2, *len);

    //int len = ReaderReceive(answer, par);

    if(crypted == CRYPT_ALL) {
        if(*len == 1) {
            FURI_LOG_I("ASTRA", "len = 1");
            uint16_t res = 0;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 0)) << 0;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 1)) << 1;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 2)) << 2;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 3)) << 3;
            answer[0] = res;
        } else {
            FURI_LOG_I("ASTRA", "len = %d", *len);
            for(pos = 0; pos < *len; pos++) answer[pos] = crypto1_byte(pcs, 0x00, 0) ^ answer[pos];
        }
    }
    return *len;
}

// mifare classic commands
int mifare_classic_auth(
    struct Crypto1State* pcs,
    uint32_t uid,
    uint8_t blockNo,
    uint8_t keyType,
    uint64_t ui64Key,
    uint8_t isNested) {
    return mifare_classic_authex(pcs, uid, blockNo, keyType, ui64Key, isNested, NULL, NULL);
}

int mifare_classic_authex(
    struct Crypto1State* pcs,
    uint32_t uid,
    uint8_t blockNo,
    uint8_t keyType,
    uint64_t ui64Key,
    uint8_t isNested,
    uint32_t* ntptr,
    uint32_t* timing) {
    int len; 
    uint32_t pos, nt, ntpp; // Supplied tag nonce
    uint8_t par[1] = {0x00}; // parity
    uint8_t nr[4]; // reader nonce
    uint8_t* rx_buff;
    uint16_t* rx_len;
    uint8_t* rx_parbits;
    uint8_t mf_nr_ar[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t receivedAnswer[MAX_MIFARE_FRAME_SIZE] = {0x00};

    // "random" reader nonce:
    num_to_bytes(prng_successor(DWT->CYCCNT, 32), 4, nr);

    uid = 0x67B48AB3;

    // Transmit MIFARE_CLASSIC_AUTH
    len = mifare_sendcmd_short(
        pcs, isNested, 0x60 + (keyType & 0x01), blockNo, receivedAnswer, timing);
    //if(len != 4) return 1;

    nr[0] = 0x11;
    nr[1] = 0x11;
    nr[2] = 0x11;
    nr[3] = 0x11;

    // Save the tag nonce (nt)
    nt = bytes_to_num(receivedAnswer, 4);

    nt = 0x01200145;

    FURI_LOG_I("MIFARE", "nt is %04x \n", nt);

    FURI_LOG_I("MIFARE", "nr[0] is %02x \n", nr[0]);
    FURI_LOG_I("MIFARE", "nr[1] is %02x \n", nr[1]);
    FURI_LOG_I("MIFARE", "nr[2] is %02x \n", nr[2]);
    FURI_LOG_I("MIFARE", "nr[3] is %02x \n", nr[3]); 

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

    // some statistic
    if(!ntptr)
    FURI_LOG_I("MIFARE", "auth uid: %08x | nr: %08x | nt: %08x", uid, *nr, nt);
    // save Nt
    if(ntptr) *ntptr = nt;

    // Generate (encrypted) nr+parity by loading it into the cipher (Nr)
    par[0] = 0;
    for(pos = 0; pos < 4; pos++) {
        mf_nr_ar[pos] = crypto1_byte(pcs, nr[pos], 0) ^ nr[pos];
        par[0] |= (((filter(pcs->odd) ^ oddparity8(nr[pos])) & 0x01) << (7 - pos));
        FURI_LOG_I("PARITY", "curr par1 is %02x", par[0]);
    }

    FURI_LOG_I("MIFARE", "par_pre is  = %02x", par[0]);
    // Skip 32 bits in pseudo random generator
    nt = prng_successor(nt, 32);

    //  ar+parity
    for(pos = 4; pos < 8; pos++) {
        nt = prng_successor(nt, 8);
        mf_nr_ar[pos] = crypto1_byte(pcs, 0x00, 0) ^ (nt & 0xff);
        par[0] |= (((filter(pcs->odd) ^ oddparity8(nt & 0xff)) & 0x01) << (7 - pos));
        FURI_LOG_I("PARITY", "curr par2 is %02x", par[0]);
    }

    // Transmit reader nonce and reader answer
    //ReaderTransmitPar(mf_nr_ar, sizeof(mf_nr_ar), par, NULL);
    FURI_LOG_I("ASTRA", "mf_nr_ar[0] = %02X", mf_nr_ar[0]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[1] = %02X", mf_nr_ar[1]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[2] = %02X", mf_nr_ar[2]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[3] = %02X", mf_nr_ar[3]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[4] = %02X", mf_nr_ar[4]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[5] = %02X", mf_nr_ar[5]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[6] = %02X", mf_nr_ar[6]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[7] = %02X", mf_nr_ar[7]);
    FURI_LOG_I("ASTRA", "par[0] = %02X", par[0]);
    
    furi_hal_nfc_raw_parbits_exchange(mf_nr_ar, sizeof(mf_nr_ar), par, &rx_buff, &rx_len, &rx_parbits, false);
    // save standard timeout
    //uint32_t save_timeout = iso14a_get_timeout();

    // set timeout for authentication response
    //if(save_timeout > 103) iso14a_set_timeout(103);

    // Receive 4 byte tag answer
    //len = ReaderReceive(receivedAnswer, receivedAnswerPar);

    //iso14a_set_timeout(save_timeout);

    if(!len) {
        FURI_LOG_I("MIFARE", "Authentication failed. Card timeout");
        return 2;
    }

    ntpp = prng_successor(nt, 32) ^ crypto1_word(pcs, 0, 0);

    if(ntpp != bytes_to_num(receivedAnswer, 4)) {
        FURI_LOG_I("MIFARE", "Authentication failed. Error card response. Expected %08x but received %08x", ntpp, bytes_to_num(receivedAnswer, 4));
        return 3;
    }
    return 0;
    }

uint16_t mf_classic_read_block(struct Crypto1State* pcs, uint32_t uid, uint8_t blockNo, uint8_t *blockData) {
    int len;
    uint8_t bt[2] = {0x00, 0x00};
    uint8_t receivedAnswer[MAX_MIFARE_FRAME_SIZE] = {0x00};


    len = (int) mifare_sendcmd_short_no_crc(
        pcs, 1, ISO14443A_CMD_READBLOCK, blockNo, receivedAnswer);
    if(len == 1) {
        printf("Cmd Error %02x", receivedAnswer[0]);
        return 0;
    }
    if(len != 18) {
        printf("wrong response len %d (expected 18)", len);
        return 0;
    }

    memcpy(bt, receivedAnswer + 16, 2);
    AddCrc14A(receivedAnswer, 16);
    if(bt[0] != receivedAnswer[16] || bt[1] != receivedAnswer[17]) {
        printf("CRC response error");
        return 0;
    }

    memcpy(blockData, receivedAnswer, 16);
    return sizeof(blockData);
}

void MifareReadBlock(uint8_t blockNo, uint8_t keyType, uint64_t ui64Key, uint8_t uid) {

    // variables
    //uint8_t dataoutbuf[16] = {0x00};
    //uint8_t uid[10] = {0x00};
    //uint32_t cuid = 0;

    struct Crypto1State mpcs = {0, 0};
    struct Crypto1State* pcs;
    pcs = &mpcs;


    if(mifare_classic_auth(pcs, uid, blockNo, keyType, ui64Key, AUTH_FIRST)) {
        FURI_LOG_I("MFC", "Auth error");
    };

    

    crypto1_deinit(pcs);

    FURI_LOG_I("MFC", "READ BLOCK FINISHED");

    //reply_ng(CMD_HF_MIFARE_READBL, status, dataoutbuf, 16);
}