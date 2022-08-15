#pragma once

#include <furi_hal_nfc.h>
#include <storage/storage.h>
#include "lib/nfc/protocols/nfca.h"
#include "lib/nfc/protocols/nfc_util.h"

typedef struct NfcDebugPcapWorker NfcDebugPcapWorker;

NfcDebugPcapWorker* nfc_debug_pcap_alloc(Storage* storage);

void nfc_debug_pcap_free(NfcDebugPcapWorker* instance);

/** Prepare tx/rx context for debug pcap logging, if enabled.
 *
 * @param      instance NfcDebugPcapWorker* instance, can be NULL
 * @param      tx_rx   TX/RX context to log
 * @param      is_picc if true, record Flipper as PICC, else PCD.
 */
void nfc_debug_pcap_prepare_tx_rx(
    NfcDebugPcapWorker* instance,
    FuriHalNfcTxRxContext* tx_rx,
    bool is_picc);

/*
   Traceformat:
   32 bits timestamp (little endian)
   16 bits duration (little endian)
   15 bits data length (little endian) (0x7FFF)
   1 bit isResponse (0=reader to tag, 1=tag to reader)
   data length Bytes data
   x Bytes parity,  where x == ceil(data length/8)
*/

// typedef struct {
//     uint32_t timestamp;
//     uint16_t duration;
//     uint16_t data_len : 15;
//     bool isResponse : 1;
//     uint8_t frame[];
//     // data_len         bytes of data
//     // ceil(data_len/8) bytes of parity
// } PACKED tracelog_hdr_t;

// #define TRACELOG_HDR_LEN sizeof(tracelog_hdr_t)
// #define TRACELOG_PARITY_LEN(x) (((x)->data_len - 1) / 8 + 1)