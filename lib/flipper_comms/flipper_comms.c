#include "flipper_comms.h"

#define TAG "FlipperComms"

uint8_t* flipper_comms_compose(
    uint8_t* message,
    size_t message_len,
    uint8_t chunk_count,
    uint8_t chunk_index) {
    furi_assert(message_len > 0);
    furi_assert(message_len <= MESSAGE_MAX_LEN);
    if(message_len > MESSAGE_MAX_LEN) {
        furi_crash("Comms message too long");
    }
    uint8_t* message_buffer = malloc(PARAMS_COUNT + message_len);
    message_buffer[VERSION_POS] = 0x01; // Version: 0.0.1
    message_buffer[TYPE_POS] = 0x00; // Type: RFU
    message_buffer[TTL_POS] = 0x08; // TTL: 8
    message_buffer[CHUNK_COUNT_POS] = chunk_count; // Chunk count
    message_buffer[CHUNK_NUMBER_POS] = chunk_index; // Chunk number
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
    FURI_LOG_W(TAG, "Sending message, len: %d", message_len);

    furi_hal_delay_ms(1);
    // Send message
    for(int i = 0; i < 5; i++) {
        subghz_tx_rx_worker_write(worker->subghz_txrx, message, message_len);
        furi_hal_delay_ms(1);
    }

    return true;
}

bool flipper_comms_send(FlipperCommsWorker* worker, uint8_t* message, size_t message_len) {
    // Send message to all devices
    // Check TTL
    if(message[TTL_POS] > 0) {
        // Substract 1 from TTL
        message[TTL_POS]--;
        // Calculate checksum and place it to message_buffer[CRC_POS]
        uint8_t crc = 0;
        for(size_t i = 0; i < message_len; i++) {
            if(i == CRC_POS) {
                continue;
            }
            crc += message[i];
        }
        message[CRC_POS] = crc;
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
    // Start subghz worker
    worker->state = WORKER_BUSY;
    uint32_t message_len;
    // Read message
    for(int i = 0; i < 1; i++) {
        furi_hal_delay_ms(1);
        // Clear message buffer
        for(size_t i = 0; i < MESSAGE_MAX_LEN; i++) {
            message[i] = 0;
        }
        message_len = subghz_tx_rx_worker_read(worker->subghz_txrx, message, COMPOSED_MAX_LEN);
        furi_hal_delay_ms(1);
        if(message_len > 0) {
            // Check CRC
            uint8_t crc = 0;
            for(size_t i = 0; i < message_len; i++) {
                if(i == CRC_POS) {
                    continue;
                }
                crc += message[i];
            }
            if(crc == message[CRC_POS]) {
                FURI_LOG_W(TAG, "Received message with crc: %02x", crc);
                continue;
            } else {
                FURI_LOG_W(TAG, "CRC error: %02x != %02x", crc, message[CRC_POS]);
            }
            // Check if we have a chunked message
            if(message[CHUNK_COUNT_POS] > 1) {
                furi_crash("Chunked messages are not supported");
            } else {
                // We have a regular message
                FURI_LOG_W(TAG, "Received non-chunked message: %u", message_len);
                worker->state = WORKER_IDLE;

                return message_len;
            }
        }
    }

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
    if(message[VERSION_POS] != 0x01) {
        FURI_LOG_E(TAG, "Invalid version");
        return message_buffer;
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

bool flipper_comms_set_listen_callback(FlipperCommsWorker* comms, void* callback) {
    comms->callback = (CommsRxCb)callback;
    return true;
}

int32_t flipper_comms_listen_service(void* context) {
    FlipperCommsWorker* comms = (FlipperCommsWorker*)context;
    uint8_t message[COMPOSED_MAX_LEN];
    uint32_t recv_len = 0;
    CommsRxCb callback = comms->callback;
    while(1) {
        if(!comms->running) {
            break;
        } else {
            recv_len = flipper_comms_read(comms, message);
            if(recv_len > 0) {
                furi_hal_delay_ms(1);
                flipper_comms_send(comms, message, recv_len);
                FURI_LOG_W(
                    TAG,
                    "Calling back with message %02X %02X %02X %02X %02X %02X",
                    message[0],
                    message[1],
                    message[2],
                    message[3],
                    message[4],
                    message[5]);
                FURI_LOG_W(TAG, "Message len: %u", recv_len);
                //callback(flipper_comms_decode(message, recv_len), recv_len);
                callback(message, recv_len);
                furi_hal_delay_ms(100);
            } else {
                furi_hal_delay_ms(100);
                FURI_LOG_D(TAG, "No message received");
            }
        }
    }
    return 0;
}

bool flipper_comms_start_listen_thread(FlipperCommsWorker* comms) {
    // Start subghz worker
    comms->subghz_txrx = subghz_tx_rx_worker_alloc();
    subghz_tx_rx_worker_start(comms->subghz_txrx, comms->frequency);
    // Start thread
    comms->thread = furi_thread_alloc();
    furi_thread_set_name(comms->thread, "FlipperComms");
    furi_thread_set_stack_size(comms->thread, 2048);
    furi_thread_set_callback(comms->thread, (FuriThreadCallback)flipper_comms_listen_service);
    furi_thread_set_context(comms->thread, comms);
    comms->running = true;
    furi_thread_start(comms->thread);
    return true;
}

bool flipper_comms_stop_listen_thread(FlipperCommsWorker* comms) {
    // Stop subghz worker
    subghz_tx_rx_worker_stop(comms->subghz_txrx);
    subghz_tx_rx_worker_free(comms->subghz_txrx);
    comms->subghz_txrx = NULL;

    // Stop thread
    comms->running = false;
    furi_thread_join(comms->thread);
    furi_thread_free(comms->thread);
    return true;
}

uint32_t flipper_comms_adv_service(void* context) {
    FlipperCommsWorker* comms = (FlipperCommsWorker*)context;
    while(1) {
        flipper_comms_send_by_chunk(comms, comms->adv_message, comms->adv_message_len);
        furi_hal_delay_ms(100);
        if(!comms->running) {
            break;
        }
    }
    return 0;
}

bool flipper_comms_start_adv_thread(FlipperCommsWorker* comms) {
    // Start subghz worker
    comms->subghz_txrx = subghz_tx_rx_worker_alloc();
    subghz_tx_rx_worker_start(comms->subghz_txrx, comms->frequency);
    // Start thread
    comms->thread = furi_thread_alloc();
    furi_thread_set_name(comms->thread, "FlipperComms");
    furi_thread_set_stack_size(comms->thread, 2048);
    furi_thread_set_callback(comms->thread, (FuriThreadCallback)flipper_comms_adv_service);
    furi_thread_set_context(comms->thread, comms);
    comms->running = true;
    furi_thread_start(comms->thread);
    return true;
}

bool flipper_comms_stop_adv_thread(FlipperCommsWorker* comms) {
    // Stop subghz worker
    subghz_tx_rx_worker_stop(comms->subghz_txrx);
    subghz_tx_rx_worker_free(comms->subghz_txrx);
    comms->subghz_txrx = NULL;
    // Stop thread
    comms->running = false;
    furi_thread_join(comms->thread);
    furi_thread_free(comms->thread);
    return true;
}

bool flipper_comms_free(FlipperCommsWorker* worker) {
    flipper_comms_stop_listen_thread(worker);
    flipper_comms_stop_adv_thread(worker);
    free(worker);
    return true;
}

bool flipper_comms_send_by_chunk(FlipperCommsWorker* worker, uint8_t* message, size_t message_len) {
    // If the message does not need to be split, send it directly
    if(message_len <= (MESSAGE_MAX_LEN + 1)) {
        FURI_LOG_W(TAG, "Sending non-chunked message");
        return flipper_comms_send(
            worker, flipper_comms_compose(message, message_len, 1, 0), message_len + PARAMS_COUNT);
    } else {
        // Crash
        FURI_LOG_E(TAG, "Message too long: %u", message_len);
        furi_crash("fuck");
    }
}
