#pragma once
#include <furi.h>
#include <lib/subghz/subghz_tx_rx_worker.h>

#define COMPOSED_MAX_LEN 50

// Message structure:
// {CRC, Length, TTL, Message}
// CRC: 1 byte
// Length: 1 byte
// TTL: 1 byte
// Message: COMPOSED_MAX_LEN - all other bytes (50 - 3 = 47 bytes)

// Create defines
#define FF_POS 0
#define CRC_POS 1
#define LENGTH_POS 2
#define TTL_POS 3

#define PARAMS_LEN 5 // 2 for FF in the beginning and in the end

#define MESSAGE_MAX_LEN (COMPOSED_MAX_LEN - PARAMS_LEN)

typedef struct {
    uint32_t frequency;
    void* callback;
    SubGhzTxRxWorker* subghz_txrx;
    FuriThread* thread;
    volatile bool running;
    uint8_t state;
    uint8_t* adv_message;
    uint32_t adv_message_len;
    uint8_t* last_messages[20];
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
uint8_t flipper_comms_checksum(uint8_t* data, uint8_t len);
