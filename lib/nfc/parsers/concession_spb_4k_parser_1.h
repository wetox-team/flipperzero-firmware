#pragma once

#include "nfc_supported_card.h"

bool concession_spb_4k_parser_verify_1(NfcWorker* nfc_worker, FuriHalNfcTxRxContext* tx_rx);

bool concession_spb_4k_parser_read_1(NfcWorker* nfc_worker, FuriHalNfcTxRxContext* tx_rx);

bool concession_spb_4k_parser_parse_1(NfcDeviceData* dev_data);