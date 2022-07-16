#pragma once
#include <furi.h>
#include <lib/subghz/subghz_tx_rx_worker.h>
#include <notification/notification.h>

#include "flipper_comms.h"

typedef struct {
    char name[16];
} TomodachiObject;
typedef struct {
    TomodachiObject tomodachi_known[20];
    uint8_t tomodachi_known_count;
    TomodachiObject me;
    SubGhzTxRxWorker* subghz_txrx;
    FlipperCommsWorker* flipper_comms;
    char last_message[COMPOSED_MAX_LEN];
    char prev_message[COMPOSED_MAX_LEN];
    NotificationApp* notifications;
} Tomodachi;
