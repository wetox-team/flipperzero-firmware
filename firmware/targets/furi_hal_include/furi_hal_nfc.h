/**
 * @file furi_hal_nfc.h
 * NFC HAL API
 */

#pragma once

#include <rfal_nfc.h>
#include <st_errno.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FURI_HAL_NFC_UID_MAX_LEN 10

#define FURI_HAL_NFC_TXRX_DEFAULT                                                    \
    ((uint32_t)RFAL_TXRX_FLAGS_CRC_TX_AUTO | (uint32_t)RFAL_TXRX_FLAGS_CRC_RX_REMV | \
     (uint32_t)RFAL_TXRX_FLAGS_NFCIP1_OFF | (uint32_t)RFAL_TXRX_FLAGS_AGC_ON |       \
     (uint32_t)RFAL_TXRX_FLAGS_PAR_RX_REMV | (uint32_t)RFAL_TXRX_FLAGS_PAR_TX_AUTO | \
     (uint32_t)RFAL_TXRX_FLAGS_NFCV_FLAG_AUTO)

#define FURI_HAL_NFC_TXRX_RAW                                                          \
    ((uint32_t)RFAL_TXRX_FLAGS_CRC_TX_MANUAL | (uint32_t)RFAL_TXRX_FLAGS_CRC_RX_REMV | \
     (uint32_t)RFAL_TXRX_FLAGS_NFCIP1_OFF | (uint32_t)RFAL_TXRX_FLAGS_AGC_ON |         \
     (uint32_t)RFAL_TXRX_FLAGS_PAR_RX_REMV | (uint32_t)RFAL_TXRX_FLAGS_PAR_TX_NONE |   \
     (uint32_t)RFAL_TXRX_FLAGS_NFCV_FLAG_AUTO)

typedef bool (*FuriHalNfcEmulateCallback)(
    uint8_t* buff_rx,
    uint16_t buff_rx_len,
    uint8_t* buff_tx,
    uint16_t* buff_tx_len,
    uint32_t* flags,
    void* context);

/** Init nfc
 */
void furi_hal_nfc_init();

/** Check if nfc worker is busy
 *
 * @return     true if busy
 */
bool furi_hal_nfc_is_busy();

/** NFC field on
 */
void furi_hal_nfc_field_on();

/** NFC field off
 */
void furi_hal_nfc_field_off();

/** NFC start sleep
 */
void furi_hal_nfc_start_sleep();

/** NFC stop sleep
 */
void furi_hal_nfc_exit_sleep();

/** NFC poll
 *
 * @param      dev_list    pointer to rfalNfcDevice buffer
 * @param      dev_cnt     pointer device count
 * @param      timeout     timeout in ms
 * @param      deactivate  deactivate flag
 *
 * @return     true on success
 */
bool furi_hal_nfc_detect(
    rfalNfcDevice** dev_list,
    uint8_t* dev_cnt,
    uint32_t timeout,
    bool deactivate);

/** NFC listen
 *
 * @param      uid                 pointer to uid buffer
 * @param      uid_len             uid length
 * @param      atqa                pointer to atqa
 * @param      sak                 sak
 * @param      activate_after_sak  activate after sak flag
 * @param      timeout             timeout in ms
 *
 * @return     true on success
 */
bool furi_hal_nfc_listen(
    uint8_t* uid,
    uint8_t uid_len,
    uint8_t* atqa,
    uint8_t sak,
    bool activate_after_sak,
    uint32_t timeout);

bool furi_hal_nfc_emulate_nfca(
    uint8_t* uid,
    uint8_t uid_len,
    uint8_t* atqa,
    uint8_t sak,
    FuriHalNfcEmulateCallback callback,
    void* context,
    uint32_t timeout);

/** Get first command from reader after activation in emulation mode
 *
 * @param      rx_buff  pointer to receive buffer
 * @param      rx_len   receive buffer length
 *
 * @return     true on success
 */
bool furi_hal_nfc_get_first_frame(uint8_t** rx_buff, uint16_t** rx_len);

/** NFC data exchange
 *
 * @param      tx_buff     transmit buffer
 * @param      tx_len      transmit buffer length
 * @param      rx_buff     receive buffer
 * @param      rx_len      receive buffer length
 * @param      deactivate  deactivate flag
 *
 * @return     ST ReturnCode
 */
ReturnCode furi_hal_nfc_data_exchange(
    uint8_t* tx_buff,
    uint16_t tx_len,
    uint8_t** rx_buff,
    uint16_t** rx_len,
    bool deactivate);

/** NFC data exchange without the CRC checksum on TX
 *
 * @param      tx_buff     transmit buffer
 * @param      tx_len      transmit buffer length
 * @param      rx_buff     receive buffer
 * @param      rx_len      receive buffer length
 * @param      deactivate  deactivate flag
 *
 * @return     ST ReturnCode
 */
ReturnCode furi_hal_nfc_data_no_crc_exchange(
    uint8_t* tx_buff,
    uint16_t tx_len,
    uint8_t** rx_buff,
    uint16_t** rx_len,
    bool deactivate);

/** NFC raw bit stream exchange. If last byte is only used partially (e.g. bit length is 5), least significant bits are used
 *
 * @param      tx_buff_bitstream  transmit buffer, with raw bits to send
 * @param      tx_bit_count       transmit buffer length in bits (may contain partial bytes)
 * @param      rx_buff_bitstream  receive buffer to get raw received bits
 * @param      rx_bit_count       received bit count
 * @param      deactivate         deactivate flag
 *
 * @return     ST ReturnCode
 */

ReturnCode furi_hal_nfc_raw_bitstream_exchange(
    uint8_t* tx_buff_bitstream,
    uint16_t tx_bit_count,
    uint8_t** rx_buff_bitstream,
    uint16_t** rx_bit_count,
    bool deactivate);

/** NFC raw bit stream exchange. If last byte is only used partially (e.g. bit length is 5), least significant bits are used
 *
 * @param      tx_buff_bitstream  transmit buffer, with raw bits to send
 * @param      tx_bit_count       transmit buffer length in bits (may contain partial bytes)
 * @param      rx_buff_bitstream  receive buffer to get raw received bits
 * @param      rx_bit_count       received bit count
 * @param      deactivate         deactivate flag
 * @param      flags              transmit and receive flags
 *
 * @return     ST ReturnCode
 */
ReturnCode furi_hal_nfc_custom_flags_exchange(
    uint8_t* tx_buff,
    uint16_t tx_len,
    uint8_t** rx_buff,
    uint16_t** rx_len,
    bool deactivate,
    uint32_t flags);

/** NFC raw exchange in parity bytes format: {data_byte, 0x80 or 0x00, data_byte, 0x80 or 0x00, ...}. E.g. 93 20 (ANTICOLL) would be {0x93, 0x80, 0x20, 0x00}
 *
 * @param      tx_buff_parbytes  transmit buffer, in parity bytes format
 * @param      tx_buff_len       transmit buffer length, must be even
 * @param      rx_buff_parbytes  receive buffer gets data in parity bytes format
 * @param      rx_buff_len       received buffer length, will be even
 * @param      deactivate        deactivate flag
 *
 * @return     ST ReturnCode
 */
ReturnCode furi_hal_nfc_raw_parbytes_exchange(
    uint8_t* tx_buff_parbytes,
    uint16_t tx_buff_len,
    uint8_t** rx_buff_parbytes,
    uint16_t** rx_buff_len,
    bool deactivate);

/** NFC raw exchange in detached parity bits format. Bits are packed MSB first. E.g. 50 00 57 CD (HALT) would be buff="\x50\x00\x57\xcd" parbits={0b11000000} (only first 1100 is used for 4 data bytes)
 *
 * @param      tx_buff_parbytes  transmit buffer, in parity bytes format
 * @param      tx_buff_len       transmit buffer length, must be even
 * @param      rx_buff_parbytes  receive buffer gets data in parity bytes format
 * @param      rx_buff_len       received buffer length, will be even
 * @param      deactivate        deactivate flag
 *
 * @return     ST ReturnCode
 */
ReturnCode furi_hal_nfc_raw_parbits_exchange(
    uint8_t* tx_buff,
    uint16_t tx_len,
    uint8_t* tx_parity_bits,
    uint8_t** rx_buff,
    uint16_t** rx_len,
    uint8_t** rx_parity_bits,
    bool deactivate);

/** NFC deactivate and start sleep
 */
void furi_hal_nfc_deactivate();

void furi_hal_nfc_stop();

uint16_t furi_hal_nfc_parbits2bitstream(
    uint8_t* buff,
    uint16_t len,
    uint8_t* parity_bits,
    uint8_t* output_bitstream);

#ifdef __cplusplus
}
#endif


