#include "mifare_classic.h"
#include <furi.h>
#include <furi_hal.h>
#include <stdlib.h>

#define RFAL_TXRX_FLAGS_FIRST_AUTH                                                   \
    ((uint32_t)RFAL_TXRX_FLAGS_CRC_TX_AUTO | (uint32_t)RFAL_TXRX_FLAGS_CRC_RX_KEEP | \
     (uint32_t)RFAL_TXRX_FLAGS_NFCIP1_OFF | (uint32_t)RFAL_TXRX_FLAGS_AGC_ON |       \
     (uint32_t)RFAL_TXRX_FLAGS_PAR_RX_REMV | (uint32_t)RFAL_TXRX_FLAGS_PAR_TX_AUTO | \
     (uint32_t)RFAL_TXRX_FLAGS_NFCV_FLAG_AUTO)

#define RFAL_TXRX_FLAGS_NOCRC_TX                                                       \
    ((uint32_t)RFAL_TXRX_FLAGS_CRC_TX_MANUAL | (uint32_t)RFAL_TXRX_FLAGS_CRC_RX_KEEP | \
     (uint32_t)RFAL_TXRX_FLAGS_NFCIP1_OFF | (uint32_t)RFAL_TXRX_FLAGS_AGC_ON |         \
     (uint32_t)RFAL_TXRX_FLAGS_PAR_RX_REMV | (uint32_t)RFAL_TXRX_FLAGS_PAR_TX_AUTO |   \
     (uint32_t)RFAL_TXRX_FLAGS_NFCV_FLAG_AUTO)

#define AddCrc14A(data, len) \
    compute_crc(CRC_14443_A, (data), (len), (data) + (len), (data) + (len) + 1)

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
    //AddCrc14A(dcmd, 2);
    memcpy(ecmd, dcmd, sizeof(dcmd));
    uint8_t* answer2 = answer;
    if(pcs && crypted) {
        for(pos = 0; pos < 2; pos++) {
            ecmd[pos] = crypto1_byte(pcs, 0x00, 0) ^ dcmd[pos];
        }
        //ReaderTransmitPar(ecmd, sizeof(ecmd), par, timing);
        //FURI_LOG_I("ASTRA", "ecmd_reg = %02X", ecmd[1]);
        //furi_hal_nfc_data_exchange(ecmd, sizeof(ecmd), &answer2, &len, false);
        furi_hal_nfc_custom_flags_exchange(
            ecmd, sizeof(ecmd), &answer2, &len, false, RFAL_TXRX_FLAGS_FIRST_AUTH);
    } else {
        //ReaderTransmit(dcmd, sizeof(dcmd), timing);
        //FURI_LOG_I("ASTRA", "dcmd_reg = %02X", dcmd[1]);
        //furi_hal_nfc_data_exchange(dcmd, sizeof(dcmd), &answer2, &len, false);
        furi_hal_nfc_custom_flags_exchange(
            ecmd, sizeof(ecmd), &answer2, &len, false, RFAL_TXRX_FLAGS_FIRST_AUTH);
    }

    if(len == 0) {
        FURI_LOG_E("MIFARE", "No response from card received");
        return 1;
    }
    if(len != 0) {
        //FURI_LOG_I("MIFARE", "prelen = %d", *len);
        memcpy(answer, answer2, *len);
        //FURI_LOG_I("MIFARE", "postlen = %d", *len);
    }
    //int len = ReaderReceive(answer, par);

    if(crypted == CRYPT_ALL) {
        if(*len == 1) {
            //FURI_LOG_I("ASTRA2", "len = 1");
            uint16_t res = 0;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 0)) << 0;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 1)) << 1;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 2)) << 2;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 3)) << 3;
            answer[0] = res;
        } else {
            //FURI_LOG_I("ASTRA2", "len = %d", *len);
            for(pos = 0; pos < *len; pos++) answer[pos] = crypto1_byte(pcs, 0x00, 0) ^ answer[pos];
        }
    }
    return *len;
}

// send 2 byte commands without automatic crc
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
        // FURI_LOG_I("MIFARE", "ecmd_no_crc = %02X", ecmd[1]);
        // FURI_LOG_I("MIFARE", "par_no_crc = %02X", par[0]);
        furi_hal_nfc_raw_parbits_exchange(
            ecmd, sizeof(ecmd), par, &answer2, &len, &rx_parbits, false);
    } else {
        //ReaderTransmit(dcmd, sizeof(dcmd), timing);
        // FURI_LOG_I("MIFARE", "dcmd_no_crc = %02X", dcmd[1]);
        // FURI_LOG_I("MIFARE", "par_no_crc = %02X", par[0]);
        furi_hal_nfc_custom_flags_exchange(
            ecmd, sizeof(dcmd), &answer2, &len, false, RFAL_TXRX_FLAGS_NOCRC_TX);
    }
    //FURI_LOG_I("MIFARE", "len = %d", *len);
    memcpy(answer, answer2, *len);
    //
    //int len = ReaderReceive(answer, par);

    if(crypted == CRYPT_ALL) {
        if(*len == 1) {
            //FURI_LOG_I("ASTRA1", "len = 1");
            uint16_t res = 0;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 0)) << 0;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 1)) << 1;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 2)) << 2;
            res |= (crypto1_bit(pcs, 0, 0) ^ BIT(answer[0], 3)) << 3;
            answer[0] = res;
        } else {
            //FURI_LOG_I("ASTRA1", "len = %d", *len);
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
    uint8_t nr[4] = {0x00, 0x00, 0x00, 0x00}; // reader nonce
    uint8_t* rx_buff;
    uint16_t* rx_len;
    uint8_t* rx_parbits;
    uint8_t mf_nr_ar[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t receivedAnswer[MAX_MIFARE_FRAME_SIZE] = {0x00};
    uint8_t receivedAt[MAX_MIFARE_FRAME_SIZE] = {0x00};

    // "random" reader nonce:
    num_to_bytes(prng_successor(DWT->CYCCNT, 32), 4, nr);

    // Transmit MIFARE_CLASSIC_AUTH
    len = mifare_sendcmd_short_no_crc(
        pcs, isNested, 0x60 + (keyType & 0x01), blockNo, receivedAnswer);
    //if(len != 4) return 1;

    // Save the tag nonce (nt)
    nt = bytes_to_num(receivedAnswer, 4);

    // FURI_LOG_I("MIFARE", "nt is %04x \n", nt);

    // FURI_LOG_I("MIFARE", "nr[0] is %02x", nr[0]);
    // FURI_LOG_I("MIFARE", "nr[1] is %02x", nr[1]);
    // FURI_LOG_I("MIFARE", "nr[2] is %02x", nr[2]);
    // FURI_LOG_I("MIFARE", "nr[3] is %02x", nr[3]);

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

    //FURI_LOG_I("MIFARE", "auth uid: %08x | nr: %08x | nt: %08x", uid, *nr, nt);
    // save Nt
    if(ntptr) *ntptr = nt;

    // Generate (encrypted) nr+parity by loading it into the cipher (Nr)
    par[0] = 0;
    for(pos = 0; pos < 4; pos++) {
        mf_nr_ar[pos] = crypto1_byte(pcs, nr[pos], 0) ^ nr[pos];
        par[0] |= (((filter(pcs->odd) ^ oddparity8(nr[pos])) & 0x01) << (7 - pos));
        //FURI_LOG_I("PARITY", "curr par1 is %02x", par[0]);
    }

    //FURI_LOG_I("MIFARE", "par_pre is  = %02x", par[0]);
    // Skip 32 bits in pseudo random generator
    nt = prng_successor(nt, 32);

    //  ar+parity
    for(pos = 4; pos < 8; pos++) {
        nt = prng_successor(nt, 8);
        mf_nr_ar[pos] = crypto1_byte(pcs, 0x00, 0) ^ (nt & 0xff);
        par[0] |= (((filter(pcs->odd) ^ oddparity8(nt & 0xff)) & 0x01) << (7 - pos));
        //FURI_LOG_I("PARITY", "curr par2 is %02x", par[0]);
    }

    // Transmit reader nonce and reader answer
    //ReaderTransmitPar(mf_nr_ar, sizeof(mf_nr_ar), par, NULL);
    // FURI_LOG_I("ASTRA", "mf_nr_ar[0] = %02X", mf_nr_ar[0]);
    // FURI_LOG_I("ASTRA", "mf_nr_ar[1] = %02X", mf_nr_ar[1]);
    // FURI_LOG_I("ASTRA", "mf_nr_ar[2] = %02X", mf_nr_ar[2]);
    // FURI_LOG_I("ASTRA", "mf_nr_ar[3] = %02X", mf_nr_ar[3]);
    // FURI_LOG_I("ASTRA", "mf_nr_ar[4] = %02X", mf_nr_ar[4]);
    // FURI_LOG_I("ASTRA", "mf_nr_ar[5] = %02X", mf_nr_ar[5]);
    // FURI_LOG_I("ASTRA", "mf_nr_ar[6] = %02X", mf_nr_ar[6]);
    // FURI_LOG_I("ASTRA", "mf_nr_ar[7] = %02X", mf_nr_ar[7]);
    // FURI_LOG_I("ASTRA", "par[0] = %02X", par[0]);

    furi_hal_nfc_raw_parbits_exchange(
        mf_nr_ar, sizeof(mf_nr_ar), par, &rx_buff, &rx_len, &rx_parbits, false);
    // save standard timeout
    //uint32_t save_timeout = iso14a_get_timeout();

    // set timeout for authentication response
    //if(save_timeout > 103) iso14a_set_timeout(103);

    // Receive 4 byte tag answer
    //len = ReaderReceive(receivedAnswer, receivedAnswerPar);

    //iso14a_set_timeout(save_timeout);
    //
    memcpy(receivedAt, rx_buff, MAX_MIFARE_FRAME_SIZE);

    if(!len) {
        //FURI_LOG_I("MIFARE", "Authentication failed. Card timeout");
        return 2;
    }

    ntpp = prng_successor(nt, 32) ^ crypto1_word(pcs, 0, 0);

    if(ntpp != bytes_to_num(receivedAt, 4)) {
        // FURI_LOG_I(
        //     "MIFARE",
        //     "Authentication failed. Error card response. Expected %08x but received %08x",
        //     ntpp,
        //     bytes_to_num(receivedAt, 4));
        return 3;
    }

    //FURI_LOG_I("MIFARE", "Authentication success! at is %08x", bytes_to_num(receivedAt, 4));
    return 0;

    free(rx_buff);
}

uint16_t mf_classic_read_block(
    struct Crypto1State* pcs,
    uint32_t uid,
    uint8_t blockNo,
    uint8_t* blockData) {
    int len;
    uint8_t bt[2] = {0x00, 0x00};
    uint8_t receivedAnswer[MAX_MIFARE_FRAME_SIZE] = {0x00};

    len =
        (int)mifare_sendcmd_short_no_crc(pcs, 1, ISO14443A_CMD_READBLOCK, blockNo, receivedAnswer);
    if(len == 1) {
        //printf("Cmd Error %02x", receivedAnswer[0]);
        return 0;
    }
    if(len != 18) {
        //printf("wrong response len %d (expected 18)", len);
        return 0;
    }

    memcpy(bt, receivedAnswer + 16, 2);
    AddCrc14A(receivedAnswer, 16);
    if(bt[0] != receivedAnswer[16] || bt[1] != receivedAnswer[17]) {
        //printf("CRC response error");
        return 0;
    }

    memcpy(blockData, receivedAnswer, 16);
    // FURI_LOG_I(
    //     "MIFARE",
    //     "Read Block %02x successfully, received data starts with %02x",
    //     blockNo,
    //     *blockData);

    // printf("\n");
    // printf("\n");
    // for(int i = 0; i < 16; i++) {
    //     printf("%02x", blockData[i]);
    //     printf(":");
    // }
    // printf("\n");

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

void mf_classic_prepare_emulation(MifareClassicDevice* mf_classic_emulate, MifareClassicData* data) {
    mf_classic_emulate->data = *data;
    mf_classic_emulate->data_changed = false;

    // TODO
}

uint16_t mf_classic_prepare_emulation_response(
    uint8_t* buff_rx,
    uint16_t len_rx,
    uint8_t* buff_tx,
    MifareClassicDevice* mf_classic_emulate) {
    // TODO

    return 0x00;
    //return tx_bits;
}

//-----------------------------------------------------------------------------
// acquire encrypted nonces in order to perform the attack described in
// Carlo Meijer, Roel Verdult, "Ciphertext-only Cryptanalysis on Hardened
// Mifare Classic Cards" in Proceedings of the 22nd ACM SIGSAC Conference on
// Computer and Communications Security, 2015
//-----------------------------------------------------------------------------
// void MifareAcquireEncryptedNonces(uint32_t arg0, uint32_t arg1, uint32_t flags, uint8_t* datain) {
//     struct Crypto1State mpcs = {0, 0};
//     struct Crypto1State* pcs;
//     pcs = &mpcs;

//     uint8_t uid[10] = {0x00};
//     uint8_t receivedAnswer[MAX_MIFARE_FRAME_SIZE] = {0x00};
//     uint8_t par_enc[1] = {0x00};
//     uint8_t buf[PM3_CMD_DATA_SIZE] = {0x00};

//     uint64_t ui64Key = bytes_to_num(datain, 6);
//     uint32_t cuid = 0;
//     int16_t isOK = 0;
//     uint16_t num_nonces = 0;
//     uint8_t nt_par_enc = 0;
//     uint8_t cascade_levels = 0;
//     uint8_t blockNo = arg0 & 0xff;
//     uint8_t keyType = (arg0 >> 8) & 0xff;
//     uint8_t targetBlockNo = arg1 & 0xff;
//     uint8_t targetKeyType = (arg1 >> 8) & 0xff;
//     bool initialize = flags & 0x0001;
//     bool slow = flags & 0x0002;
//     bool field_off = flags & 0x0004;
//     bool have_uid = false;

//     BigBuf_free();
//     BigBuf_Clear_ext(false);
//     clear_trace();
//     set_tracing(false);

//     if(initialize) iso14443a_setup(FPGA_HF_ISO14443A_READER_LISTEN);

//     LED_C_ON();

//     for(uint16_t i = 0; i <= PM3_CMD_DATA_SIZE - 9;) {
//         // Test if the action was cancelled
//         if(BUTTON_PRESS()) {
//             isOK = 2;
//             field_off = true;
//             break;
//         }

//         if(!have_uid) { // need a full select cycle to get the uid first
//             iso14a_card_select_t card_info;
//             if(!iso14443a_select_card(uid, &card_info, &cuid, true, 0, true)) {
//                 if(g_dbglevel >= DBG_ERROR)
//                     Dbprintf("AcquireEncryptedNonces: Can't select card (ALL)");
//                 continue;
//             }
//             switch(card_info.uidlen) {
//             case 4:
//                 cascade_levels = 1;
//                 break;
//             case 7:
//                 cascade_levels = 2;
//                 break;
//             case 10:
//                 cascade_levels = 3;
//                 break;
//             default:
//                 break;
//             }
//             have_uid = true;
//         } else { // no need for anticollision. We can directly select the card
//             if(!iso14443a_fast_select_card(uid, cascade_levels)) {
//                 if(g_dbglevel >= DBG_ERROR)
//                     Dbprintf("AcquireEncryptedNonces: Can't select card (UID)");
//                 continue;
//             }
//         }

//         if(slow) SpinDelayUs(HARDNESTED_PRE_AUTHENTICATION_LEADTIME);

//         uint32_t nt1;
//         if(mifare_classic_authex(pcs, cuid, blockNo, keyType, ui64Key, AUTH_FIRST, &nt1, NULL)) {
//             if(g_dbglevel >= DBG_ERROR) Dbprintf("AcquireEncryptedNonces: Auth1 error");
//             continue;
//         }

//         // nested authentication
//         uint16_t len = mifare_sendcmd_short(
//             pcs,
//             AUTH_NESTED,
//             0x60 + (targetKeyType & 0x01),
//             targetBlockNo,
//             receivedAnswer,
//             par_enc,
//             NULL);

//         // wait for the card to become ready again
//         CHK_TIMEOUT();

//         if(len != 4) {
//             if(g_dbglevel >= DBG_ERROR)
//                 Dbprintf("AcquireEncryptedNonces: Auth2 error len=%d", len);
//             continue;
//         }

//         num_nonces++;
//         if(num_nonces % 2) {
//             memcpy(buf + i, receivedAnswer, 4);
//             nt_par_enc = par_enc[0] & 0xf0;
//         } else {
//             nt_par_enc |= par_enc[0] >> 4;
//             memcpy(buf + i + 4, receivedAnswer, 4);
//             memcpy(buf + i + 8, &nt_par_enc, 1);
//             i += 9;
//         }
//     }

//     crypto1_deinit(pcs);
//     reply_old(CMD_ACK, isOK, cuid, num_nonces, buf, sizeof(buf));

//     FURI_LOG_I("MIFARE", "AcquireEncryptedNonces finished");
// }
