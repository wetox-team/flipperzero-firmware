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
/*
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
    //uint8_t par[1] = {0x00};
    uint8_t nr[4];
    uint8_t* rx_buff;
    uint16_t* rx_len;
    uint8_t mf_nr_ar[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t receivedAnswer[MAX_MIFARE_FRAME_SIZE] = {0x00};

    // "random" reader nonce:
    //num_to_bytes(prng_successor(DWT->CYCCNT, 32), 4, nr);
    FURI_LOG_I("ASTRA", "nr[0] = %02X", nr[0]);
    FURI_LOG_I("ASTRA", "nr[1] = %02X", nr[1]);
    FURI_LOG_I("ASTRA", "nr[2] = %02X", nr[2]);
    FURI_LOG_I("ASTRA", "nr[3] = %02X", nr[3]); 

    nr[0] = 0xB5;
    nr[1] = 0x84;
    nr[2] = 0x2B;
    nr[3] = 0x49;

    FURI_LOG_I("ASTRA", "---------");

    FURI_LOG_I("ASTRA", "nr[0] = %02X", nr[0]);
    FURI_LOG_I("ASTRA", "nr[1] = %02X", nr[1]);
    FURI_LOG_I("ASTRA", "nr[2] = %02X", nr[2]);
    FURI_LOG_I("ASTRA", "nr[3] = %02X", nr[3]);
    FURI_LOG_I("ASTRA", "nr[4] = %02X", nr[4]);


    // Transmit MIFARE_CLASSIC_AUTH
    len = mifare_sendcmd_short(
        pcs, isNested, 0x60 + (keyType & 0x01), blockNo, receivedAnswer, timing);
    //if(len != 4) return 1;

    // Save the tag nonce (nt)
    FURI_LOG_I("ASTRA", "received answer[0] = %02X", receivedAnswer[0]);
    FURI_LOG_I("ASTRA", "received answer[1] = %02X", receivedAnswer[1]);
    FURI_LOG_I("ASTRA", "received answer[2] = %02X", receivedAnswer[2]);
    FURI_LOG_I("ASTRA", "received answer[3] = %02X", receivedAnswer[3]); 
    FURI_LOG_I("ASTRA", "received answer[4] = %02X", receivedAnswer[4]);
    FURI_LOG_I("ASTRA", "received answer[5] = %02X", receivedAnswer[5]);
    FURI_LOG_I("ASTRA", "length = %d", len);

    nt = bytes_to_num(receivedAnswer, 4);

    FURI_LOG_I("ASTRA", "nt = %08lx", nt);

    nt = 0x01200145;

    FURI_LOG_I("ASTRA", "nt = %08lx", nt);

    //  ----------------------------- crypto1 create
    if(isNested) crypto1_deinit(pcs);

    // Init cipher with key
    crypto1_init(pcs, ui64Key);

    if(isNested == AUTH_NESTED) {
        // decrypt nt with help of new key
        FURI_LOG_I("ASTRA", "encrypted nt = %d", nt);
        nt = crypto1_word(pcs, nt ^ uid, 1) ^ nt;
        FURI_LOG_I("ASTRA", "decrypted nt");
    } else {
        // Load (plain) uid^nt into the cipher
        FURI_LOG_I("ASTRA", "plain nt = %d", nt);
        crypto1_word(pcs, nt ^ uid, 0);
    }

    // some statistic
    FURI_LOG_I("ASTRA", "auth uid: %08lx | nr: %08lx | nt: %08lx", uid, nr, nt);

    // save Nt
    if(ntptr) *ntptr = nt;

    // Generate (encrypted) nr+parity by loading it into the cipher (Nr)
    //par[0] = 0;
    for(pos = 0; pos < 4; pos++) {
        mf_nr_ar[pos] = crypto1_byte(pcs, nr[pos], 0) ^ nr[pos];
        //par[0] |= (((filter(pcs->odd) ^ oddparity8(nr[pos])) & 0x01) << (7 - pos));
    }


    // Skip 32 bits in pseudo random generator
    nt = prng_successor(nt, 32);

    //  ar+parity
    for(pos = 4; pos < 8; pos++) {
        nt = prng_successor(nt, 8);
        mf_nr_ar[pos] = crypto1_byte(pcs, 0x00, 0) ^ (nt & 0xff);
        //par[0] |= (((filter(pcs->odd) ^ oddparity8(nt & 0xff)) & 0x01) << (7 - pos));
    }
    // Transmit reader nonce and reader answer
    //ReaderTransmitPar(mf_nr_ar, sizeof(mf_nr_ar), par, NULL);
    FURI_LOG_I("ASTRA", "mf_nr_ar[0] = %02X", mf_nr_ar[0]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[1] = %02X", mf_nr_ar[1]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[2] = %02X", mf_nr_ar[2]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[3] = %02X", mf_nr_ar[3]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[4] = %02X", mf_nr_ar[4]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[5] = %02X", mf_nr_ar[5]);

    //mf_nr_ar[2] = mf_nr_ar[4];
    //mf_nr_ar[3] = mf_nr_ar[5];
    //mf_nr_ar[0] = 0x00;
    //mf_nr_ar[1] = 0x00;
    //mf_nr_ar[2] = 0x00;
    //mf_nr_ar[3] = 0x00;
    //mf_nr_ar[4] = 0x00;
    //mf_nr_ar[5] = 0x00;
    //mf_nr_ar[6] = 0x00;
    //mf_nr_ar[7] = 0x00;

    furi_hal_nfc_raw_exchange(mf_nr_ar, 8, &rx_buff, &rx_len, false);

    // save standard timeout
    //uint32_t save_timeout = iso14a_get_timeout();

    // set timeout for authentication response
    //if(save_timeout > 103) iso14a_set_timeout(103);

    // Receive 4 byte tag answer
    //len = ReaderReceive(receivedAnswer, receivedAnswerPar);

    //iso14a_set_timeout(save_timeout);


    ntpp = prng_successor(nt, 32) ^ crypto1_word(pcs, 0, 0);

    if(ntpp != bytes_to_num(rx_buff, 4)) {
        printf("Authentication failed. Error card response");
        return 3;
    }
    return 0;
}

*/

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
    uint8_t mf_nr_ar[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t mf_nr_ar_par[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t mf_test[] = {0x80, 0x60, 0x78, 0x1e, 0x1f, 0x87, 0xe7, 0xf9, 0xff};
    uint8_t receivedAnswer[MAX_MIFARE_FRAME_SIZE] = {0x00};

    // "random" reader nonce:
    //num_to_bytes(prng_successor(GetTickCount(), 32), 4, nr);
    nr[0] = 0xB5;
    nr[1] = 0x84;
    nr[2] = 0x2B;
    nr[3] = 0x49;

    uid = 0x67B48AB3;

    // Transmit MIFARE_CLASSIC_AUTH
    len = mifare_sendcmd_short(
        pcs, isNested, 0x60 + (keyType & 0x01), blockNo, receivedAnswer, timing);
    //if(len != 4) return 1;

    // Save the tag nonce (nt)
    nt = bytes_to_num(receivedAnswer, 4);

    nt = 0x01200145;

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
    FURI_LOG_I("MIFARE", "auth uid: %08x | nr: %08x | nt: %08x", uid, nr, nt);

    // save Nt
    if(ntptr) *ntptr = nt;

    // Generate (encrypted) nr+parity by loading it into the cipher (Nr)
    //par[0] = 0;
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
    FURI_LOG_I("ASTRA", "mf_nr_ar[0] = %02X", mf_nr_ar[0]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[1] = %02X", mf_nr_ar[1]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[2] = %02X", mf_nr_ar[2]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[3] = %02X", mf_nr_ar[3]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[4] = %02X", mf_nr_ar[4]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[5] = %02X", mf_nr_ar[5]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[6] = %02X", mf_nr_ar[6]);
    FURI_LOG_I("ASTRA", "mf_nr_ar[7] = %02X", mf_nr_ar[7]);
    FURI_LOG_I("ASTRA", "par[0] = %02X", par[0]);


    // for (int i = 0; i < 8; i++){
    //     for (int j=0; j < 8; j++){
    //         if (curr_len == 8){
    //             mf_nr_ar_par[curr_byte] = buff;
    //             buff = 0;
    //             curr_len = 0;
    //             FURI_LOG_I("ASTRA_DBG", "mf_nr_ar_par[%d] = %02X", curr_byte, mf_nr_ar_par[curr_byte]);
    //             curr_byte++;
    //         }
    //         int curr_bit = mf_nr_ar[i] >> j;
    //         buff <<= 1;
    //         buff |= curr_bit & 1;
    //         curr_len++;
    //     }

    //     if (curr_len == 8){
    //         mf_nr_ar_par[curr_byte] = buff;
    //         buff = 0;
    //         curr_len = 0;
    //         FURI_LOG_I("ASTRA_DBG", "mf_nr_ar_par[%d] = %02X", curr_byte, mf_nr_ar_par[curr_byte]);
    //         curr_byte++;
    //     }
        
    //     buff <<= 1;
    //     par[0] >>= 1;
    //     buff |= par[0] & 1;
    //     curr_len++;
    // }
    // mf_nr_ar_par[curr_byte] = buff;
    // buff = 0;
    // curr_len = 0;
    // FURI_LOG_I("ASTRA_DBG_POST", "mf_nr_ar_par[%d] = %02X", curr_byte, mf_nr_ar_par[curr_byte]);
    // curr_byte++;
    par[0] = 0xAB;
    int bits[72];
    int curr_bit = 0;
    int cur_par_bit = 0;
    int par_bit;
    int bit;


    for (int i = 0; i < 8; i++){
        for (int j = 0; j < 8; j++){
            bit = (mf_nr_ar[i] >> (7-j)) & 1;
            bits[curr_bit++] = bit & 1;
            // FURI_LOG_I("ASTRA", "mf_bit[%d][%d] = %d", i, 7-j, bit & 1);
        }
        par_bit = (par[0] >> (7-cur_par_bit++)) & 1;
        // FURI_LOG_I("ASTRA", "par_bit[%d] = %d", i, par_bit & 1);
        bits[curr_bit++] =  par_bit & 1;
    }

    printf("[ASTRA log array] = [");
    for (int i = 0; i < 9; i++){
        printf("[");
        for (int j = 0; j < 8; j++){
            printf("%d", bits[i*8 + j] & 1);
            if (j != 7) printf(", ");
        }
        printf("]");
        if (i != 8) printf(", ");
    }
    printf("]\n");
    

    FURI_LOG_I("ASTRA", "HUI");
    for (int i = 0; i < 9; i++){
        for (int j = 0; j < 8; j++){
            mf_nr_ar_par[i] <<= 1;
            if (bits[i*8 + j] == 1)
                mf_nr_ar_par[i] |= 1;

            // FURI_LOG_I("ASTRA", "%d", bits[i*8 + j]);
        }
        FURI_LOG_I("ASTRA", "%02X ", mf_nr_ar_par[i]);
    }

    FURI_LOG_I("ASTRA", "mf_nr_ar_par FULL");

    for (int i = 0; i < 9; i++)
        FURI_LOG_I("ASTRA", "mf_nr_ar_par[%d] = %02X", i, mf_nr_ar_par[i]);
    

    char ch_bits[72];
    for (int i = 0; i < 72; i++){
        if (bits[i] == 1)
            ch_bits[i] = '1';
        else ch_bits[i] = '0';
    }
    FURI_LOG_I("CHAR ARRAY", "%s", ch_bits);
    FURI_LOG_I("CHAR ARRAY", "%s", ch_bits);

    // num_to_bytes(strtoll(ch_bits, NULL, 2), 9, mf_nr_ar_par);



    // LOG PART

    int new_bits[72];
    for (int i = 0; i < 9; i++){
        for (int j = 0; j < 8; j++){
            bit = (mf_nr_ar_par[i] >> (7-j)) & 1;
            new_bits[curr_bit++] = bit & 1;
            // FURI_LOG_I("ASTRA", "mf_bit[%d][%d] = %d", i, 7-j, bit & 1);
        }
    }
    FURI_LOG_I("DONE", "done!");
    printf("[ASTRA log array1] = [");
    for (int i = 0; i < 9; i++){
        printf("[");
        for (int j = 0; j < 8; j++){
            printf("%d", new_bits[i*8 + j] & 1);
            if (j != 7) printf(", ");
        }
        printf("]");
        if (i != 8) printf(", ");
    }
    printf("]\n");

    // END LOG PART


    // int bits[72];

    // for (int i = 0; i < 8; i++) {
    //     for (int j = 0; j < 8; j++) {
    //         bits[j + 9 * i] = (mf_nr_ar[i] >> j) & 1;
    //     }
    //     bits[8 + i * 9] = (par[0] >> i) & 1;
    // }

    // for(int i = 0; i < 9; i++) {
    //     for(int j = 7; j >= 0; j--) {
    //         mf_nr_ar_par[i] <<= 1;
    //         mf_nr_ar_par[i] |= bits[i * 8 + j] & 1;

    //     }
    // }
    //memcpy(mf_nr_ar_par, mf_test, 9);

    FURI_LOG_I("ASTRA", "you wont see this");
    FURI_LOG_I("ASTRA", "mf_nr_ar_par[0] = %02X", mf_nr_ar_par[0]);
    FURI_LOG_I("ASTRA", "mf_nr_ar_par[1] = %02X", mf_nr_ar_par[1]);
    FURI_LOG_I("ASTRA", "mf_nr_ar_par[2] = %02X", mf_nr_ar_par[2]);
    FURI_LOG_I("ASTRA", "mf_nr_ar_par[3] = %02X", mf_nr_ar_par[3]);
    FURI_LOG_I("ASTRA", "mf_nr_ar_par[4] = %02X", mf_nr_ar_par[4]);
    FURI_LOG_I("ASTRA", "mf_nr_ar_par[5] = %02X", mf_nr_ar_par[5]);
    FURI_LOG_I("ASTRA", "mf_nr_ar_par[6] = %02X", mf_nr_ar_par[6]);
    FURI_LOG_I("ASTRA", "mf_nr_ar_par[7] = %02X", mf_nr_ar_par[7]);
    FURI_LOG_I("ASTRA", "mf_nr_ar_par[8] = %02X", mf_nr_ar_par[8]);

    furi_hal_nfc_raw_exchange(mf_test, 9, &rx_buff, &rx_len, false);
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
        FURI_LOG_I("MIFARE", "Authentication failed. Error card response");
        return 3;
    }
    return 0;
    }

uint16_t mf_classic_read_block(struct Crypto1State* pcs, uint32_t uid, uint8_t* dest, uint8_t start_block) {
    int len;
    uint8_t bt[2] = {0x00, 0x00};
    uint8_t receivedAnswer[MAX_MIFARE_FRAME_SIZE] = {0x00};


    len = (int) mifare_sendcmd_short(
        pcs, 1, ISO14443A_CMD_READBLOCK, start_block, receivedAnswer, NULL);
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

    memcpy(dest, receivedAnswer, 16);
    return sizeof(dest);
}

void MifareReadBlock(uint8_t blockNo, uint8_t keyType, uint64_t ui64Key) {

    // variables
    //uint8_t dataoutbuf[16] = {0x00};
    //uint8_t uid[10] = {0x00};
    uint32_t cuid = 0;

    struct Crypto1State mpcs = {0, 0};
    struct Crypto1State* pcs;
    pcs = &mpcs;


    if(mifare_classic_auth(pcs, cuid, blockNo, keyType, ui64Key, AUTH_FIRST)) {
        FURI_LOG_I("MFC", "Auth error");
    };

    

    crypto1_deinit(pcs);

    FURI_LOG_I("MFC", "READ BLOCK FINISHED");

    //reply_ng(CMD_HF_MIFARE_READBL, status, dataoutbuf, 16);
}