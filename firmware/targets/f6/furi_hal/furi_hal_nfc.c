#include "furi_hal_nfc.h"
#include <st25r3916.h>

#define TAG "FuriHalNfc"

static const uint32_t clocks_in_ms = 64 * 1000;

void furi_hal_nfc_init() {
    ReturnCode ret = rfalNfcInitialize();
    if(ret == ERR_NONE) {
        furi_hal_nfc_start_sleep();
        FURI_LOG_I(TAG, "Init OK");
    } else {
        FURI_LOG_W(TAG, "Initialization failed, RFAL returned: %d", ret);
    }
}

bool furi_hal_nfc_is_busy() {
    return rfalNfcGetState() != RFAL_NFC_STATE_IDLE;
}

void furi_hal_nfc_field_on() {
    furi_hal_nfc_exit_sleep();
    st25r3916TxRxOn();
}

void furi_hal_nfc_field_off() {
    st25r3916TxRxOff();
    furi_hal_nfc_start_sleep();
}

void furi_hal_nfc_start_sleep() {
    rfalLowPowerModeStart();
}

void furi_hal_nfc_exit_sleep() {
    rfalLowPowerModeStop();
}

bool furi_hal_nfc_detect(
    rfalNfcDevice** dev_list,
    uint8_t* dev_cnt,
    uint32_t timeout,
    bool deactivate) {
    furi_assert(dev_list);
    furi_assert(dev_cnt);

    rfalLowPowerModeStop();
    rfalNfcState state = rfalNfcGetState();
    if(state == RFAL_NFC_STATE_NOTINIT) {
        rfalNfcInitialize();
    }
    rfalNfcDiscoverParam params;
    params.compMode = RFAL_COMPLIANCE_MODE_EMV;
    params.techs2Find = RFAL_NFC_POLL_TECH_A | RFAL_NFC_POLL_TECH_B | RFAL_NFC_POLL_TECH_F |
                        RFAL_NFC_POLL_TECH_V | RFAL_NFC_POLL_TECH_AP2P | RFAL_NFC_POLL_TECH_ST25TB;
    params.totalDuration = 1000;
    params.devLimit = 3;
    params.wakeupEnabled = false;
    params.wakeupConfigDefault = true;
    params.nfcfBR = RFAL_BR_212;
    params.ap2pBR = RFAL_BR_424;
    params.maxBR = RFAL_BR_KEEP;
    params.GBLen = RFAL_NFCDEP_GB_MAX_LEN;
    params.notifyCb = NULL;

    uint32_t start = DWT->CYCCNT;
    rfalNfcDiscover(&params);
    while(state != RFAL_NFC_STATE_ACTIVATED) {
        rfalNfcWorker();
        state = rfalNfcGetState();
        FURI_LOG_T(TAG, "Current state %d", state);
        if(state == RFAL_NFC_STATE_POLL_ACTIVATION) {
            start = DWT->CYCCNT;
            continue;
        }
        if(state == RFAL_NFC_STATE_POLL_SELECT) {
            rfalNfcSelect(0);
        }
        if(DWT->CYCCNT - start > timeout * clocks_in_ms) {
            rfalNfcDeactivate(true);
            FURI_LOG_T(TAG, "Timeout");
            return false;
        }
        osThreadYield();
    }
    rfalNfcGetDevicesFound(dev_list, dev_cnt);
    if(deactivate) {
        rfalNfcDeactivate(false);
        rfalLowPowerModeStart();
    }
    return true;
}

bool furi_hal_nfc_listen(
    uint8_t* uid,
    uint8_t uid_len,
    uint8_t* atqa,
    uint8_t sak,
    bool activate_after_sak,
    uint32_t timeout) {
    rfalNfcState state = rfalNfcGetState();
    if(state == RFAL_NFC_STATE_NOTINIT) {
        rfalNfcInitialize();
    } else if(state >= RFAL_NFC_STATE_ACTIVATED) {
        rfalNfcDeactivate(false);
    }
    rfalLowPowerModeStop();
    rfalNfcDiscoverParam params = {
        .compMode = RFAL_COMPLIANCE_MODE_NFC,
        .techs2Find = RFAL_NFC_LISTEN_TECH_A,
        .totalDuration = 1000,
        .devLimit = 1,
        .wakeupEnabled = false,
        .wakeupConfigDefault = true,
        .nfcfBR = RFAL_BR_212,
        .ap2pBR = RFAL_BR_424,
        .maxBR = RFAL_BR_KEEP,
        .GBLen = RFAL_NFCDEP_GB_MAX_LEN,
        .notifyCb = NULL,
        .activate_after_sak = activate_after_sak,
    };
    params.lmConfigPA.nfcidLen = uid_len;
    memcpy(params.lmConfigPA.nfcid, uid, uid_len);
    params.lmConfigPA.SENS_RES[0] = atqa[0];
    params.lmConfigPA.SENS_RES[1] = atqa[1];
    params.lmConfigPA.SEL_RES = sak;
    rfalNfcDiscover(&params);

    uint32_t start = DWT->CYCCNT;
    while(state != RFAL_NFC_STATE_ACTIVATED) {
        rfalNfcWorker();
        state = rfalNfcGetState();
        if(DWT->CYCCNT - start > timeout * clocks_in_ms) {
            rfalNfcDeactivate(true);
            return false;
        }
        osThreadYield();
    }
    return true;
}

bool furi_hal_nfc_get_first_frame(uint8_t** rx_buff, uint16_t** rx_len) {
    ReturnCode ret =
        rfalNfcDataExchangeStart(NULL, 0, rx_buff, rx_len, 0, RFAL_TXRX_FLAGS_DEFAULT);
    return ret == ERR_NONE;
}

ReturnCode furi_hal_nfc_data_exchange(
    uint8_t* tx_buff,
    uint16_t tx_len,
    uint8_t** rx_buff,
    uint16_t** rx_len,
    bool deactivate) {
    furi_assert(rx_buff);
    furi_assert(rx_len);

    ReturnCode ret;
    rfalNfcState state = RFAL_NFC_STATE_ACTIVATED;
    ret = rfalNfcDataExchangeStart(tx_buff, tx_len, rx_buff, rx_len, 0, RFAL_TXRX_FLAGS_DEFAULT);
    if(ret != ERR_NONE) {
        return ret;
    }
    uint32_t start = DWT->CYCCNT;
    while(state != RFAL_NFC_STATE_DATAEXCHANGE_DONE) {
        rfalNfcWorker();
        state = rfalNfcGetState();
        ret = rfalNfcDataExchangeGetStatus();
        if(ret > ERR_SLEEP_REQ) {
            return ret;
        }
        if(ret == ERR_BUSY) {
            if(DWT->CYCCNT - start > 1000 * clocks_in_ms) {
                return ERR_TIMEOUT;
            }
            continue;
        } else {
            start = DWT->CYCCNT;
        }
        taskYIELD();
    }
    if(deactivate) {
        rfalNfcDeactivate(false);
        rfalLowPowerModeStart();
    }
    return ERR_NONE;
}

ReturnCode furi_hal_nfc_raw_bitstream_exchange(
    uint8_t* tx_buff,
    uint16_t tx_bit_len,
    uint8_t** rx_buff,
    uint16_t** rx_bit_len,
    bool deactivate) {
    furi_assert(rx_buff);
    furi_assert(rx_bit_len);

    ReturnCode ret;
    rfalNfcState state = RFAL_NFC_STATE_ACTIVATED;
    ret =
        rfalNfcDataExchangeStart(tx_buff, tx_bit_len, rx_buff, rx_bit_len, 0, RFAL_TXRX_FLAGS_RAW);
    if(ret != ERR_NONE) {
        return ret;
    }
    uint32_t start = DWT->CYCCNT;
    while(state != RFAL_NFC_STATE_DATAEXCHANGE_DONE) {
        rfalNfcWorker();
        state = rfalNfcGetState();
        ret = rfalNfcDataExchangeGetStatus();
        if(ret > ERR_SLEEP_REQ) {
            return ret;
        }
        if(ret == ERR_BUSY) {
            if(DWT->CYCCNT - start > 1000 * clocks_in_ms) {
                return ERR_TIMEOUT;
            }
            continue;
        } else {
            start = DWT->CYCCNT;
        }
        taskYIELD();
    }
    if(deactivate) {
        rfalNfcDeactivate(false);
        rfalLowPowerModeStart();
    }
    return ERR_NONE;
}

ReturnCode furi_hal_nfc_custom_flags_exchange(
    uint8_t* tx_buff,
    uint16_t tx_len,
    uint8_t** rx_buff,
    uint16_t** rx_len,
    bool deactivate,
    uint32_t flags) {
    furi_assert(rx_buff);
    furi_assert(rx_len);

    ReturnCode ret;
    rfalNfcState state = RFAL_NFC_STATE_ACTIVATED;
    ret = rfalNfcDataExchangeStart(tx_buff, tx_len, rx_buff, rx_len, 0, flags);
    if(ret != ERR_NONE) {
        return ret;
    }
    uint32_t start = DWT->CYCCNT;
    while(state != RFAL_NFC_STATE_DATAEXCHANGE_DONE) {
        rfalNfcWorker();
        state = rfalNfcGetState();
        ret = rfalNfcDataExchangeGetStatus();
        if(ret > ERR_SLEEP_REQ) {
            return ret;
        }
        if(ret == ERR_BUSY) {
            if(DWT->CYCCNT - start > 1000 * clocks_in_ms) {
                return ERR_TIMEOUT;
            }
            continue;
        } else {
            start = DWT->CYCCNT;
        }
        taskYIELD();
    }
    if(deactivate) {
        rfalNfcDeactivate(false);
        rfalLowPowerModeStart();
    }
    return ERR_NONE;
}

ReturnCode furi_hal_nfc_data_no_crc_exchange(
    uint8_t* tx_buff,
    uint16_t tx_len,
    uint8_t** rx_buff,
    uint16_t** rx_len,
    bool deactivate) {
    furi_assert(rx_buff);
    furi_assert(rx_len);

    ReturnCode ret;
    rfalNfcState state = RFAL_NFC_STATE_ACTIVATED;
    ret = rfalNfcDataNoCRCExchangeStart(tx_buff, tx_len, rx_buff, rx_len, 0);
    if(ret != ERR_NONE) {
        return ret;
    }
    uint32_t start = DWT->CYCCNT;
    while(state != RFAL_NFC_STATE_DATAEXCHANGE_DONE) {
        rfalNfcWorker();
        state = rfalNfcGetState();
        ret = rfalNfcDataExchangeGetStatus();
        if(ret > ERR_SLEEP_REQ) {
            return ret;
        }
        if(ret == ERR_BUSY) {
            if(DWT->CYCCNT - start > 1000 * clocks_in_ms) {
                return ERR_TIMEOUT;
            }
            continue;
        } else {
            start = DWT->CYCCNT;
        }
        taskYIELD();
    }
    if(deactivate) {
        rfalNfcDeactivate(false);
        rfalLowPowerModeStart();
    }
    return ERR_NONE;
}

static uint16_t furi_hal_nfc_parbytes2bitstream(
    uint8_t* buff_parbytes,
    uint16_t buff_len,
    uint8_t* output_bitstream) {
    furi_assert(buff_len % 2 == 0);

    uint16_t in_i, out_i = 0, accum = 0, accum_len = 0, bit_count = 0;
    for(in_i = 0; in_i < buff_len; in_i++) {
        if(in_i % 2 == 0) { // data byte
            accum |= buff_parbytes[in_i] << accum_len; // append entire data byte
            accum_len += 8;
        } else { // parity byte (0x80 / 0x00)
            accum |= (buff_parbytes[in_i] ? 1 : 0) << accum_len; // append one bit
            accum_len += 1;
        }
        while(accum_len >= 8) {
            output_bitstream[out_i++] = accum & 0xFF;
            accum >>= 8;
            accum_len -= 8;
            bit_count += 8;
        }
    }
    if(accum_len != 0) {
        output_bitstream[out_i++] = accum & 0xFF;
        bit_count += accum_len;
    }

    return bit_count;
}

static uint16_t furi_hal_nfc_bitstream2parbytes(
    uint8_t* buff_bitstream,
    uint16_t bit_count,
    uint8_t* output_parbytes) {
    furi_assert(bit_count % 9 == 0);

    uint16_t in_i = 0, out_len = 0, accum = 0, accum_len = 0, need_bits;
    while(bit_count > 0) {
        if(out_len % 2 == 0) { // expect data byte
            need_bits = 8;
        } else { // expect parity bit
            need_bits = 1;
        }
        if(accum_len < need_bits) {
            if(bit_count < need_bits) {
                break;
            }
            accum |= buff_bitstream[in_i++] << accum_len;
            accum_len += 8;
        }
        if(out_len % 2 == 0) { // shift off a data byte
            output_parbytes[out_len++] = accum & 0xFF;
            accum >>= 8;
            accum_len -= 8;
        } else { // shift off a parity bit
            output_parbytes[out_len++] = (accum & 1) ? 0x80 : 0x00;
            accum >>= 1;
            accum_len -= 1;
        }
        bit_count -= need_bits;
    }

    return out_len;
}

static uint8_t furi_hal_nfc_tmp_buff[RFAL_FEATURE_NFC_RF_BUF_LEN * 2];
static uint16_t furi_hal_nfc_tmp_buff_len;

ReturnCode furi_hal_nfc_raw_parbytes_exchange(
    uint8_t* tx_buff_parbytes,
    uint16_t tx_buff_len,
    uint8_t** rx_buff_parbytes,
    uint16_t** rx_buff_len,
    bool deactivate) {
    furi_assert(rx_buff_parbytes);
    furi_assert(rx_buff_len);
    furi_assert(tx_buff_len % 2 == 0);

    uint16_t tx_bit_count =
        furi_hal_nfc_parbytes2bitstream(tx_buff_parbytes, tx_buff_len, furi_hal_nfc_tmp_buff);
    uint8_t* rx_buff_bitstream;
    uint16_t* rx_bit_count;
    ReturnCode ret = furi_hal_nfc_raw_bitstream_exchange(
        furi_hal_nfc_tmp_buff, tx_bit_count, &rx_buff_bitstream, &rx_bit_count, deactivate);
    if(ret == ERR_NONE || (ret >= ERR_INCOMPLETE_BYTE && ret <= ERR_INCOMPLETE_BYTE_07)) {
        furi_hal_nfc_tmp_buff_len = furi_hal_nfc_bitstream2parbytes(
            rx_buff_bitstream, *rx_bit_count, furi_hal_nfc_tmp_buff);
        *rx_buff_parbytes = furi_hal_nfc_tmp_buff;
        *rx_buff_len = &furi_hal_nfc_tmp_buff_len;
    }
    return ret;
}

static uint16_t furi_hal_nfc_parbits2bitstream(
    uint8_t* buff,
    uint16_t len,
    uint8_t* parity_bits,
    uint8_t* output_bitstream) {
    uint16_t in_i, out_i = 0, accum = 0, accum_len = 0, bit_count = 0;
    for(in_i = 0; in_i < len; in_i++) {
        accum |= buff[in_i] << accum_len; // append data byte
        accum_len += 8;
        accum |= ((parity_bits[in_i / 8] & (0x80u >> (in_i % 8))) ? 1 : 0)
                 << accum_len; // append parity bit
        accum_len += 1;
        while(accum_len >= 8) {
            output_bitstream[out_i++] = accum & 0xFF;
            accum >>= 8;
            accum_len -= 8;
            bit_count += 8;
        }
    }
    if(accum_len != 0) {
        output_bitstream[out_i++] = accum & 0xFF;
        bit_count += accum_len;
    }

    return bit_count;
}

static uint16_t furi_hal_nfc_bitstream2parbits(
    uint8_t* buff_bitstream,
    uint16_t bit_count,
    uint8_t* output_buff,
    uint8_t* output_parity_bits) {
    uint16_t in_i = 0, out_len = 0, accum = 0, accum_len = 0, take_bits;
    while(bit_count > 0) {
        while(accum_len < 9 && bit_count > 0) {
            take_bits = 8;
            if(bit_count < take_bits) {
                take_bits = bit_count; // if only partial byte left, take what's left
            }
            accum |= (buff_bitstream[in_i++] & ((1 << take_bits) - 1)) << accum_len;
            accum_len += take_bits;
            bit_count -= take_bits;
        }

        take_bits = 8;
        if(accum_len < take_bits) {
            take_bits = accum_len;
        }
        output_buff[out_len] =
            accum & 0xFF; // shift off a data byte (automatically partial if had not enough bits)
        accum >>= take_bits;
        accum_len -= take_bits;

        if(out_len % 8 == 0) {
            output_parity_bits[out_len / 8] = 0;
        }
        output_parity_bits[out_len / 8] |=
            ((accum & 1) ? 0x80u : 0x00) >>
            (out_len %
             8); // shift off a parity bit (if no bits available, 0 will be automatically taken)
        if(accum_len >= 1) {
            accum >>= 1;
            accum_len -= 1;
        }

        out_len++;
    }

    return out_len;
}

ReturnCode furi_hal_nfc_raw_parbits_exchange(
    uint8_t* tx_buff,
    uint16_t tx_len,
    uint8_t* tx_parity_bits,
    uint8_t** rx_buff,
    uint16_t** rx_len,
    uint8_t** rx_parity_bits,
    bool deactivate) {
    furi_assert(rx_buff);
    furi_assert(rx_len);
    furi_assert(rx_parity_bits);

    uint16_t tx_bit_count =
        furi_hal_nfc_parbits2bitstream(tx_buff, tx_len, tx_parity_bits, furi_hal_nfc_tmp_buff);
    uint8_t* rx_buff_bitstream;
    uint16_t* rx_bit_count;
    ReturnCode ret = furi_hal_nfc_raw_bitstream_exchange(
        furi_hal_nfc_tmp_buff, tx_bit_count, &rx_buff_bitstream, &rx_bit_count, deactivate);
    if(ret == ERR_NONE || (ret >= ERR_INCOMPLETE_BYTE && ret <= ERR_INCOMPLETE_BYTE_07)) {
        furi_hal_nfc_tmp_buff_len = furi_hal_nfc_bitstream2parbits(
            rx_buff_bitstream, *rx_bit_count, furi_hal_nfc_tmp_buff, rx_buff_bitstream);
        *rx_buff = furi_hal_nfc_tmp_buff;
        *rx_len = &furi_hal_nfc_tmp_buff_len;
        *rx_parity_bits = rx_buff_bitstream;
    }
    return ret;
}

void furi_hal_nfc_deactivate() {
    rfalNfcDeactivate(false);
    rfalLowPowerModeStart();
}
