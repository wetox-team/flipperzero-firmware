#pragma once
#include <furi.h>
#include <lib/subghz/subghz_tx_rx_worker.h>

#define MESSAGE_MAX_LENGTH 50

typedef struct {
    const char* name;
    uint32_t xp;
} TomodachiObject;
typedef struct {
    TomodachiObject tomodachi_known[20];
    TomodachiObject me;
    SubGhzTxRxWorker* subghz_txrx;
    uint8_t recent_messages[20][MESSAGE_MAX_LENGTH];
} Tomodachi;
