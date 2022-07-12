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
    uint8_t* message_buffer = malloc(COMPOSED_MAX_LEN);
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
    worker->subghz_txrx = NULL;

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
    // Start subghz worker
    worker->state = WORKER_BUSY;
    uint32_t message_len;
    // Read message
    for(int i = 0; i < 1; i++) {
        furi_hal_delay_ms(1);
        message_len = subghz_tx_rx_worker_read(worker->subghz_txrx, message, COMPOSED_MAX_LEN);
        furi_hal_delay_ms(1);
        if(message_len > 0) {
            // Check if we have a chunked message
            if(message[CHUNK_COUNT_POS] > 1) {
                FURI_LOG_W(TAG, "Received chunked message");
                uint8_t* complete_message = malloc(CHUNKED_MESSAGE_MAX_LEN);
                size_t complete_message_len = 0;
                memcpy(complete_message, message, message_len);
                complete_message_len += message_len;
                // We need to listen for other chunks
                for(uint8_t i = 0; i < message[CHUNK_COUNT_POS]; i++) {
                    // Wait for next chunk
                    subghz_tx_rx_worker_read(worker->subghz_txrx, message, COMPOSED_MAX_LEN);
                    // Check if the chunk is more than the previous one
                    if(message[CHUNK_NUMBER_POS] > i) {
                        // Add chunk to message
                        memcpy(
                            complete_message + complete_message_len,
                            message + PARAMS_COUNT,
                            message_len - PARAMS_COUNT);
                        complete_message_len += message_len;
                    } else {
                        // We have a message with a wrong chunk number
                        FURI_LOG_W(
                            TAG,
                            "Received message with wrong chunk number: %u",
                            message[CHUNK_NUMBER_POS]);
                        for(size_t i = 0; i < 20; i++) {
                            FURI_LOG_W(TAG, " %02X", message[i]);
                        }
                        free(complete_message);
                        return 0;
                    }
                }
                worker->state = WORKER_IDLE;
                memcpy(message, complete_message, complete_message_len);
                return complete_message_len;
            } else {
                // We have a regular message
                FURI_LOG_W(TAG, "Received message: %s", message);
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

bool flipper_comms_set_callback(FlipperCommsWorker* comms, void* callback) {
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
            if(comms->state != WORKER_BUSY) {
                recv_len = flipper_comms_read(comms, message);
                if(recv_len > 0) {
                    flipper_comms_send_by_chunk(comms, message, recv_len);
                    callback(flipper_comms_decode(message, recv_len));
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

bool flipper_comms_start_listen_thread(FlipperCommsWorker* comms) {
    // Start subghz worker
    comms->subghz_txrx = subghz_tx_rx_worker_alloc();
    subghz_tx_rx_worker_start(comms->subghz_txrx, comms->frequency);
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
        if (!comms->running) {
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
    flipper_comms_stop_thread(worker);
    free(worker);
    return true;
}

bool flipper_comms_send_by_chunk(FlipperCommsWorker* worker, uint8_t* message, size_t message_len) {
    // If the message does not need to be split, send it directly
    if(message_len <= MESSAGE_MAX_LEN) {
        FURI_LOG_W(TAG, "Sending non-chunked message");
        return flipper_comms_send(
            worker, flipper_comms_compose(message, message_len, 1, 0), message_len + PARAMS_COUNT);
    } else {
        // Count the number of chunks needed to send the message
        uint8_t chunks = (message_len / MESSAGE_MAX_LEN) + 1;
        // Allocate memory for the array of chunks (2-dimensional)
        uint8_t** chunks_array = malloc(sizeof(uint8_t*) * chunks);
        for(uint8_t i = 0; i < chunks; i++) {
            chunks_array[i] = malloc(sizeof(uint8_t) * MESSAGE_MAX_LEN);
        }
        // Split the message into chunks
        for(uint8_t i = 0; i < chunks; i++) {
            for(uint8_t j = 0; j < MESSAGE_MAX_LEN; j++) {
                if((uint8_t)(i * MESSAGE_MAX_LEN + j) < message_len) {
                    chunks_array[i][j] = message[i * MESSAGE_MAX_LEN + j];
                } else {
                    chunks_array[i][j] = 0;
                }
            }
        }
        // Send the chunks
        for(uint8_t i = 0; i < chunks; i++) {
            flipper_comms_send(
                worker,
                flipper_comms_compose(chunks_array[i], MESSAGE_MAX_LEN, chunks, i),
                MESSAGE_MAX_LEN + PARAMS_COUNT);
        }
        // Free the memory allocated for the chunks
        for(uint8_t i = 0; i < chunks; i++) {
            free(chunks_array[i]);
        }
        free(chunks_array);
        return true;
    }
}
