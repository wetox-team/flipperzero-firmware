#include "flipper_comms.h"

#define TAG "FlipperComms"

FlipperCommsWorker* flipper_comms_alloc(uint32_t frequency) {
    FlipperCommsWorker* comms = malloc(sizeof(FlipperCommsWorker));
    comms->frequency = frequency;
    comms->callback = 0;
    return comms;
}

uint8_t flipper_comms_checksum(uint8_t* data, uint8_t len) {
    uint8_t checksum = 0;
    for(uint8_t i = 0; i < len; i++) {
        if(i == CRC_POS) {
            continue;
        }
        checksum += data[i];
    }
    // Invert checksum
    checksum = ~checksum;
    return checksum;
}

bool flipper_comms_check_last_messages(FlipperCommsWorker* comms, uint8_t* message) {
    for(uint8_t i = 0; i < sizeof(comms->last_messages); i++) {
        if(comms->last_messages[i] == 0) {
            comms->last_messages[i] = malloc(COMPOSED_MAX_LEN);
            FURI_LOG_W(TAG, "Writing %d bytes to last_messages[%d]", message[LENGTH_POS], i);
            memcpy(comms->last_messages[i], message, message[LENGTH_POS]);
            break;
        }
        if(memcmp(comms->last_messages[i], message, message[LENGTH_POS]) == 0) {
            FURI_LOG_W(TAG, "Message already received");
            return true;
        }
    }

    return false;
}

bool flipper_comms_sanity_check(FlipperCommsWorker* comms, uint8_t* buffer, uint8_t recv_len) {
    // Check length
    if(recv_len < buffer[LENGTH_POS]) {
        FURI_LOG_E(TAG, "Len mismatch: %d != %d", recv_len, buffer[LENGTH_POS]);
        return false;
    }

    // Check CRC
    uint8_t crc = buffer[CRC_POS];
    uint8_t calculated_crc = flipper_comms_checksum(buffer, buffer[LENGTH_POS]);
    if(crc == calculated_crc) {
        FURI_LOG_I(TAG, "CRC OK");
    } else {
        FURI_LOG_E(TAG, "CRC error, %d != %d", crc, calculated_crc);
        return false;
    }

    // Check FF in the beginning and end
    if(buffer[0] != 0xFF || buffer[buffer[LENGTH_POS] - 1] != 0xFF) {
        FURI_LOG_E(TAG, "FF error, message incomplete");
        FURI_LOG_E(TAG, "FF: %d, %d", buffer[0], buffer[buffer[LENGTH_POS] - 1]);
        for(int i = 0; i < buffer[LENGTH_POS]; i++) {
            FURI_LOG_E(TAG, "%02X", buffer[i]);
        }
        return false;
    }

    // Check if message is a duplicate
    if(flipper_comms_check_last_messages(comms, buffer)) {
        FURI_LOG_E(TAG, "Message is a duplicate");
        return false;
    }

    return true;
}

uint8_t* flipper_comms_compose(FlipperCommsWorker* comms, uint8_t* buffer, uint32_t buffer_len) {
    UNUSED(comms);
    if(buffer_len > MESSAGE_MAX_LEN) {
        FURI_LOG_E(TAG, "Message too large");
        return 0;
    }

    // Fill parameters
    uint8_t* message_composed = malloc(COMPOSED_MAX_LEN);
    uint8_t message_composed_len = PARAMS_LEN;
    // Add FF in the beginning
    message_composed[FF_POS] = 0xFF;

    // Add TTL
    message_composed[TTL_POS] = 0x08; // 8 hops

    // Fill message
    for(uint8_t i = 0; i < buffer_len; i++) {
        message_composed[message_composed_len++] = buffer[i];
    }

    // Add FF to end of message
    message_composed[message_composed_len++] = 0xFF;

    // Fill length and checksum
    message_composed[LENGTH_POS] = message_composed_len;
    message_composed[CRC_POS] =
        flipper_comms_checksum(message_composed, message_composed[LENGTH_POS]);

    return message_composed;
}

size_t flipper_comms_read(FlipperCommsWorker* comms, uint8_t* buffer) {
    //furi_delay_ms(100);
    size_t recv_len;
    for(int i = 0; i < 100; i++) {
        recv_len = subghz_tx_rx_worker_read(comms->subghz_txrx, buffer, COMPOSED_MAX_LEN);
        if(recv_len > 0) {
            if(flipper_comms_sanity_check(comms, buffer, recv_len)) {
                return recv_len;
            }
        } else {
            furi_delay_ms(1);
        }
    }
    return 0;
}

bool flipper_comms_send(FlipperCommsWorker* comms, uint8_t* buffer, size_t size) {
    uint8_t* message_composed = flipper_comms_compose(comms, buffer, size);
    for(size_t i = 0; i < 5; i++) {
        furi_delay_ms(1);
        subghz_tx_rx_worker_write(
            comms->subghz_txrx, message_composed, message_composed[LENGTH_POS]);
    }
    free(message_composed);
    return true;
}

bool flipper_comms_mesh(FlipperCommsWorker* comms, uint8_t* buffer, size_t size) {
    UNUSED(size);
    furi_assert(comms->subghz_txrx);
    furi_delay_ms(1);
    if(buffer[TTL_POS] == 0) {
        return false; // TTL == 0, drop message
    } else {
        // Decrement TTL
        buffer[TTL_POS]--;

        // Calculate new CRC
        buffer[CRC_POS] = flipper_comms_checksum(buffer, buffer[LENGTH_POS]);

        // Send message
        for(size_t i = 0; i < 5; i++) {
            furi_delay_ms(furi_hal_random_get() % 3);
            if(subghz_tx_rx_worker_is_running(comms->subghz_txrx) && comms->subghz_txrx) {
                comms->running = true;
                subghz_tx_rx_worker_write(comms->subghz_txrx, buffer, buffer[LENGTH_POS]);
                comms->running = false;
            } else {
                FURI_LOG_W(TAG, "OOP");
            }
        }
        return true;
    }
}

bool flipper_comms_set_listen_callback(FlipperCommsWorker* comms, void* callback) {
    comms->callback = (CommsRxCb)callback;
    return true;
}

int32_t flipper_comms_listen_service(void* context) {
    FlipperCommsWorker* comms = (FlipperCommsWorker*)context;
    uint8_t message[COMPOSED_MAX_LEN];
    // Set messsage to 0 to make sure we don't get a message from the previous run
    for(uint8_t i = 0; i < COMPOSED_MAX_LEN; i++) {
        message[i] = 0;
    }
    uint32_t recv_len = 0;
    CommsRxCb callback = comms->callback;
    while(1) {
        // Clear message
        for(uint8_t i = 0; i < COMPOSED_MAX_LEN; i++) {
            message[i] = 0;
        }
        if(!comms->running) {
            break;
        } else {
            recv_len = flipper_comms_read(comms, message);
            if(recv_len > 0) {
                flipper_comms_mesh(comms, message, message[LENGTH_POS]);
                if(recv_len != message[LENGTH_POS]) {
                    FURI_LOG_E(
                        TAG, "Message length error, %d != %d", recv_len, message[LENGTH_POS]);
                }
                furi_delay_ms(1);
                FURI_LOG_W(
                    TAG,
                    "Calling back with message %02X %02X %02X %02X %02X %02X",
                    message[0],
                    message[1],
                    message[2],
                    message[3],
                    message[4],
                    message[5]);
                FURI_LOG_W(TAG, "Message len: %u", message[LENGTH_POS]);
                callback(message, message[LENGTH_POS]);
            } else {
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
    // Stop thread
    comms->running = false;
    furi_thread_join(comms->thread);
    furi_thread_free(comms->thread);

    // Stop subghz worker
    while(comms->running) {
        furi_delay_ms(1);
        FURI_LOG_W(TAG, "Waiting for thread to stop");
    }
    subghz_tx_rx_worker_stop(comms->subghz_txrx);
    subghz_tx_rx_worker_free(comms->subghz_txrx);
    comms->subghz_txrx = NULL;
    return true;
}

uint32_t flipper_comms_adv_service(void* context) {
    FlipperCommsWorker* comms = (FlipperCommsWorker*)context;
    while(1) {
        if(!flipper_comms_send(comms, comms->adv_message, comms->adv_message_len)) {
            FURI_LOG_W(TAG, "Failed to send message");
            FURI_LOG_W(
                TAG,
                "Message: %02x %02x %02x %02x",
                comms->adv_message[0],
                comms->adv_message[1],
                comms->adv_message[2],
                comms->adv_message[3]);
            furi_crash("Failed to send message");
        }
        furi_delay_ms(100);
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
    for(uint8_t i = 0; i < sizeof(worker->last_messages); i++) {
        free(worker->last_messages[i]);
    }
    return true;
}
