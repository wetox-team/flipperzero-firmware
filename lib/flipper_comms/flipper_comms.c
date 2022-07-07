#include "flipper_comms.h"

#define TAG "FlipperComms"

uint8_t* flipper_comms_compose(uint8_t* message, size_t message_len) {
    furi_assert(message_len > 0);
    furi_assert(message_len <= MESSAGE_MAX_LEN);
    if(message_len > MESSAGE_MAX_LEN) {
        furi_crash("Comms message too long");
    }
    uint8_t* message_buffer = malloc(COMPOSED_MAX_LEN);
    message_buffer[VERSION_POS] = 0x01; // Version: 0.0.1
    message_buffer[TYPE_POS] = 0x00; // Type: RFU
    message_buffer[TTL_POS] = 0x08; // TTL: 8
    FURI_LOG_W(
        TAG,
        "Composing message: %02x, %02x, %02x, %02x",
        message_buffer[VERSION_POS],
        message_buffer[TYPE_POS],
        message_buffer[TTL_POS],
        message_buffer[TTL_POS + 1]);
    for(int i = 0; i < 4; i++) {
        FURI_LOG_W(TAG, "message[%d] = %02x", i, message[i]);
    }
    for(size_t i = 0; i < message_len; i++) {
        message_buffer[i + PARAMS_COUNT] = message[i];
    }
    return message_buffer;
}

bool flipper_comms_write(FlipperCommsWorker* worker, uint8_t* message, size_t message_len) {
    FURI_LOG_W(TAG, "Sending message");
    uint32_t frequency = worker->frequency;
    // Start subghz worker
    worker->state = WORKER_BUSY;
    worker->subghz_txrx = subghz_tx_rx_worker_alloc();
    subghz_tx_rx_worker_start(worker->subghz_txrx, frequency);
    furi_hal_delay_ms(1);
    // Send message
    for(int i = 0; i < 5; i++) {
        subghz_tx_rx_worker_write(worker->subghz_txrx, message, message_len);
        furi_hal_delay_ms(1);
    }
    // Stop subghz worker
    subghz_tx_rx_worker_stop(worker->subghz_txrx);
    subghz_tx_rx_worker_free(worker->subghz_txrx);
    worker->state = WORKER_IDLE;
    return true;
}

bool flipper_comms_send(FlipperCommsWorker* worker, uint8_t* message, size_t message_len) {
    // Send message to all devices
    // Check TTL
    if(message[TTL_POS] > 0) {
        // Substract 1 from TTL
        message[TTL_POS]--;
        // Send message
        FURI_LOG_W(TAG, "Sending message: %s", message);
        flipper_comms_write(worker, message, message_len);
    } else {
        FURI_LOG_W(TAG, "TTL expired: %u", message[TTL_POS]);
        for(size_t i = 0; i < 20; i++) {
            FURI_LOG_W(TAG, " %02X", message[i]);
        }
        return false;
    }
    return true;
}

uint32_t flipper_comms_read(FlipperCommsWorker* worker, uint8_t* message) {
    uint32_t frequency = worker->frequency;
    // Start subghz worker
    worker->state = WORKER_BUSY;
    worker->subghz_txrx = subghz_tx_rx_worker_alloc();
    subghz_tx_rx_worker_start(worker->subghz_txrx, frequency);
    uint32_t message_len;
    // Read message
    for(int i = 0; i < 10; i++) {
        message_len = subghz_tx_rx_worker_read(worker->subghz_txrx, message, COMPOSED_MAX_LEN);
        if(message_len > 0) {
            FURI_LOG_W(TAG, "Received message: %s", message);
            subghz_tx_rx_worker_stop(worker->subghz_txrx);
            subghz_tx_rx_worker_free(worker->subghz_txrx);
            worker->state = WORKER_IDLE;
            return message_len;
        }
    }
    // Stop subghz worker

    subghz_tx_rx_worker_stop(worker->subghz_txrx);
    subghz_tx_rx_worker_free(worker->subghz_txrx);

    worker->state = WORKER_IDLE;
    return message_len;
}

uint8_t* flipper_comms_decode(uint8_t* message, size_t message_len) {
    furi_assert(message_len > PARAMS_COUNT);
    furi_assert(message_len <= COMPOSED_MAX_LEN);
    if(message_len > COMPOSED_MAX_LEN) {
        furi_crash("Comms message too long");
    }
    uint8_t* message_buffer = malloc(MESSAGE_MAX_LEN);
    if(message[VERSION_POS] != 0x00) {
        FURI_LOG_E(TAG, "Invalid version");
        return 0;
    }
    for(size_t i = 0; i < message_len - PARAMS_COUNT; i++) {
        message_buffer[i] = message[i + PARAMS_COUNT];
    }
    return message_buffer;
}

FlipperCommsWorker* flipper_comms_alloc(uint32_t frequency) {
    FlipperCommsWorker* comms = malloc(sizeof(FlipperCommsWorker));
    comms->frequency = frequency;
    comms->callback = 0;
    return comms;
}

bool flipper_comms_set_callback(FlipperCommsWorker* comms, void* callback) {
    comms->callback = (CommsRxCb)callback;
    return true;
}

int32_t flipper_comms_service(void* context) {
    FlipperCommsWorker* comms = (FlipperCommsWorker*)context;
    uint8_t message[COMPOSED_MAX_LEN];
    CommsRxCb callback = comms->callback;
    while(1) {
        if(!comms->running) {
            break;
        } else {
            if(comms->state != WORKER_BUSY) {
                if(flipper_comms_read(comms, message) > 0) {
                    flipper_comms_send(comms, message, COMPOSED_MAX_LEN);
                    callback(flipper_comms_decode(message, COMPOSED_MAX_LEN));
                    furi_hal_delay_ms(100);
                } else {
                    furi_hal_delay_ms(100);
                    FURI_LOG_D(TAG, "No message received");
                }
            } else {
                furi_hal_delay_ms(1);
                FURI_LOG_D(TAG, "Waiting for subghz worker to finish");
            }
        }
    }
    return 0;
}

bool flipper_comms_start_thread(FlipperCommsWorker* comms) {
    // Start subghz worker
    comms->subghz_txrx = subghz_tx_rx_worker_alloc();
    // subghz_tx_rx_worker_start(comms->subghz_txrx, comms->frequency);
    // Start thread
    comms->thread = furi_thread_alloc();
    furi_thread_set_name(comms->thread, "FlipperComms");
    furi_thread_set_stack_size(comms->thread, 2048);
    furi_thread_set_callback(comms->thread, (FuriThreadCallback)flipper_comms_service);
    furi_thread_set_context(comms->thread, comms);
    comms->running = true;
    furi_thread_start(comms->thread);
    return true;
}

bool flipper_comms_stop_thread(FlipperCommsWorker* comms) {
    // Stop subghz worker
    subghz_tx_rx_worker_stop(comms->subghz_txrx);
    subghz_tx_rx_worker_free(comms->subghz_txrx);
    // Stop thread
    comms->running = false;
    furi_thread_join(comms->thread);
    furi_thread_free(comms->thread);
    return true;
}

bool flipper_comms_free(FlipperCommsWorker* worker) {
    flipper_comms_stop_thread(worker);
    free(worker);
    return true;
}
