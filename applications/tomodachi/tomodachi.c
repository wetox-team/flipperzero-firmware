#include "tomodachi.h"
#include "dolphin/helpers/dolphin_state.h"
#include "furi/check.h"
#include "furi/record.h"
#include "dolphin/dolphin.h"
#include <input/input.h>
#include <stdlib.h>
#include <furi_hal_version.h>
#include "math.h"
#include "furi_hal_rtc.h"
#include <time.h>
#include <furi_hal.h>

#include <lib/toolbox/args.h>
#include <lib/subghz/subghz_keystore.h>

#define ADVERTISE true

// Tomodachi is a service that provides a way to discover and communicate with other devices.

// We advertise our presence to other devices every 5 seconds.
// We listen for other devices advertising for the rest of the time.
// We do this over a subghz network.

#define TAG "Tomo"
#define TOMODACHI_ADV_INTERVAL_MS 300

Tomodachi* tomo_alloc() {
    Tomodachi* tomodachi = malloc(sizeof(Tomodachi));
    // Create furi record
    furi_record_create("tomodachi", tomodachi);
    // Dolphin* dolphin = furi_record_open("dolphin");
    // DolphinStats stats = dolphin_stats(dolphin);
    // DolphinStats* stats_ptr = &stats;
    // Get tomodachi name from furi
    tomodachi->me.name = furi_hal_version_get_name_ptr();
    // Print debug info
    FURI_LOG_I(TAG, "Name: %s", tomodachi->me.name);
    return tomodachi;
}

bool tomo_adv(Tomodachi* tomodachi, FlipperCommsWorker* worker) {
    // Use flipper comms to send a chunked message to all devices.
    // Get our name from furi.
    tomodachi->me.name = furi_hal_version_get_name_ptr();

    FuriHalRtcDateTime* datetime = malloc(sizeof(FuriHalRtcDateTime));
    // Clear datetime
    memset(datetime, 0, sizeof(FuriHalRtcDateTime));
    furi_hal_rtc_get_datetime(datetime);
    uint32_t timestamp = furi_hal_rtc_datetime_to_timestamp(datetime);
    // First 16 bytes are the name, next 4 are the timestamp.
    uint8_t message[20] = {0};
    // Fill message name
    sprintf((char*)message, "%s", tomodachi->me.name);
    // Fill message timestamp
    for(int i = 0; i < 4; i++) {
        message[16 + i] = (timestamp >> (8 * i)) & 0xFF;
    }
    // Log timestamp
    for(int i = 0; i < 4; i++) {
        FURI_LOG_W(TAG, "ts%d", message[16 + i]);
    }
    // Send message to all devices
    free(datetime);
    return flipper_comms_send(worker, message, sizeof(message));
}

bool tomo_callback(uint8_t* message, size_t message_len) {
    UNUSED(message);
    UNUSED(message_len);

    FURI_LOG_W(TAG, "Message received");
    FURI_LOG_W(TAG, "CRC: %d", message[CRC_POS]);
    FURI_LOG_W(TAG, "Len: %d", message[LENGTH_POS]);
    FURI_LOG_W(TAG, "TTL: %d", message[TTL_POS]);

    Tomodachi* tomodachi = furi_record_open("tomodachi");
    memcpy(tomodachi->last_message, message, message[LENGTH_POS]);
    furi_record_close("tomodachi");
    // Assemble name from message
    uint8_t* name = malloc(message[LENGTH_POS] - PARAMS_LEN);
    for(uint8_t i = 0; i < message[LENGTH_POS] - PARAMS_LEN; i++) {
        name[i] = message[i + PARAMS_LEN];
    }
    FURI_LOG_W(TAG, "Name: %s", name);
    free(name);
    // Check CRC
    uint8_t crc = message[CRC_POS];
    uint8_t calculated_crc = flipper_comms_checksum(message, message[LENGTH_POS]);
    if(crc != calculated_crc) {
        FURI_LOG_W(TAG, "CRC mismatch: %d != %d", crc, calculated_crc);
        return false;
    }
    furi_hal_delay_ms(1);
    return true;
}

uint32_t tomo_srv() {
    Tomodachi* tomodachi = tomo_alloc();
    tomodachi->flipper_comms = flipper_comms_alloc(433920000);
    // Set callback
    flipper_comms_set_listen_callback(tomodachi->flipper_comms, tomo_callback);
    // Loop forever, listening every 15 seconds and advertising every 300-400 milliseconds.
    FuriHalRtcDateTime date_time;
    UNUSED(tomodachi);
    while(1) {
        // Sleep for 15 seconds.
        // Get current time.
        furi_hal_rtc_get_datetime(&date_time);
        // Advertise
        if(ADVERTISE) {
            // Start subghz worker
            tomodachi->flipper_comms->subghz_txrx = subghz_tx_rx_worker_alloc();
            subghz_tx_rx_worker_start(
                tomodachi->flipper_comms->subghz_txrx, tomodachi->flipper_comms->frequency);

            tomo_adv(tomodachi, tomodachi->flipper_comms);
            // Stop subghz worker
            subghz_tx_rx_worker_stop(tomodachi->flipper_comms->subghz_txrx);
            subghz_tx_rx_worker_free(tomodachi->flipper_comms->subghz_txrx);
            tomodachi->flipper_comms->subghz_txrx = NULL;
        }

        // Start listening thread.
        flipper_comms_start_listen_thread(tomodachi->flipper_comms);
        furi_hal_delay_ms(furi_hal_random_get() % 1000);
        // Stop listening thread.
        flipper_comms_stop_listen_thread(tomodachi->flipper_comms);
    }
    furi_crash("Tomodachi service died");
    return 0;
}

uint32_t tomodachi_on_system_start(void* context) {
    UNUSED(context);
    return 0;
}