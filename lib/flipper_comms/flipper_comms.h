#pragma once
#include <furi.h>
#include <lib/subghz/subghz_tx_rx_worker.h>

#define WORKER_BUSY 0x01
#define WORKER_IDLE 0x00

#define VERSION 0x00
#define TTL 0x08

#define VERSION_POS 0
#define TYPE_POS 1
#define TTL_POS 2
#define CHUNK_COUNT_POS 3
#define CHUNK_NUMBER_POS 4

#define PARAMS_COUNT 5

#define COMPOSED_MAX_LEN 59
#define MESSAGE_MAX_LEN (uint8_t)(COMPOSED_MAX_LEN - PARAMS_COUNT)

#define CHUNKED_MESSAGE_MAX_LEN 512

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

typedef void (*CommsRxCb)(void* message);

FlipperCommsWorker* flipper_comms_alloc(uint32_t frequency);
bool flipper_comms_send(FlipperCommsWorker* worker, uint8_t* message, size_t message_len);
uint32_t flipper_comms_read(FlipperCommsWorker* worker, uint8_t* message);
bool flipper_comms_set_listen_callback(FlipperCommsWorker* worker, void* callback);
bool flipper_comms_start_listen_thread(FlipperCommsWorker* worker);
bool flipper_comms_stop_listen_thread(FlipperCommsWorker* worker);
bool flipper_comms_set_adv_callback(FlipperCommsWorker* worker, void* callback);
bool flipper_comms_start_adv_thread(FlipperCommsWorker* worker);
bool flipper_comms_stop_adv_thread(FlipperCommsWorker* worker);
bool flipper_comms_free(FlipperCommsWorker* worker);
uint8_t* flipper_comms_compose(
    uint8_t* message,
    size_t message_len,
    uint8_t chunk_count,
    uint8_t chunk_index);
bool flipper_comms_send_by_chunk(FlipperCommsWorker* worker, uint8_t* message, size_t message_len);
