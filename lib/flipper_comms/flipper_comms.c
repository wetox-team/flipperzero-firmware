#include "flipper_comms.h"

#define TAG "FlipperComms"

FlipperCommsWorker* flipper_comms_alloc(uint32_t frequency) {
    FlipperCommsWorker* comms = malloc(sizeof(FlipperCommsWorker));
    comms->frequency = frequency;
    comms->callback = 0;
    return comms;
}

size_t flipper_comms_read(FlipperCommsWorker* comms, uint8_t* buffer) {
    furi_hal_delay_ms(100);
    size_t recv_len;
    for (int i = 0; i < 100; i++) {
        recv_len = subghz_tx_rx_worker_read(comms->subghz_txrx, buffer, COMPOSED_MAX_LEN);
        if (recv_len > 0) {
            return recv_len;
        } else {
            furi_hal_delay_ms(1);
        }
    }
    return 0;
}

bool flipper_comms_send(FlipperCommsWorker* comms, uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < 5; i++) {
        furi_hal_delay_ms(1);
        subghz_tx_rx_worker_write(comms->subghz_txrx, buffer, size);
    }
    return true;
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
                callback(message, recv_len);
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
        if (!flipper_comms_send(comms, comms->adv_message, comms->adv_message_len)){
            FURI_LOG_W(TAG, "Failed to send message");
            FURI_LOG_W(TAG, "Message: %02x %02x %02x %02x", comms->adv_message[0], comms->adv_message[1], comms->adv_message[2], comms->adv_message[3]);
            furi_crash("Failed to send message");
        }
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
