#pragma once

#include "nfc_supported_card.h"

bool concession_spb_4k_parser_verify(NfcWorker* nfc_worker, FuriHalNfcTxRxContext* tx_rx);

bool concession_spb_4k_parser_read(NfcWorker* nfc_worker, FuriHalNfcTxRxContext* tx_rx);

bool concession_spb_4k_parser_parse(NfcDeviceData* dev_data);