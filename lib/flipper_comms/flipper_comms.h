#pragma once
#include <furi.h>
#include <lib/subghz/subghz_tx_rx_worker.h>

#define COMPOSED_MAX_LEN 50

typedef struct {
    uint32_t frequency;
    void* callback;
    SubGhzTxRxWorker* subghz_txrx;
    FuriThread* thread;
    volatile bool running;
    uint8_t state;
    uint8_t* adv_message;
    uint32_t adv_message_len;
} FlipperCommsWorker;

typedef void (*CommsRxCb)(uint8_t* message, uint32_t len);

FlipperCommsWorker* flipper_comms_alloc(uint32_t frequency);
bool flipper_comms_set_listen_callback(FlipperCommsWorker* worker, void* callback);
bool flipper_comms_start_listen_thread(FlipperCommsWorker* worker);
bool flipper_comms_stop_listen_thread(FlipperCommsWorker* worker);
bool flipper_comms_set_adv_callback(FlipperCommsWorker* worker, void* callback);
bool flipper_comms_start_adv_thread(FlipperCommsWorker* worker);
bool flipper_comms_stop_adv_thread(FlipperCommsWorker* worker);
bool flipper_comms_free(FlipperCommsWorker* worker);
size_t flipper_comms_read(FlipperCommsWorker* worker, uint8_t* buffer);
bool flipper_comms_send(FlipperCommsWorker* worker, uint8_t* buffer, size_t size);
