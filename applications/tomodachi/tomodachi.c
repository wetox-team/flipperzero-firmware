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

#include <lib/toolbox/args.h>
#include <lib/subghz/subghz_keystore.h>

#define TYPE 0
#define VERSION 1
#define TTL 2

// Tomodachi is a service that provides a way to discover and communicate with other devices.

// We advertise our presence to other devices every 5 seconds.
// We listen for other devices advertising for the rest of the time.
// We do this over a subghz network.

#define TAG "Tomo"
#define TOMODACHI_ADV_INTERVAL_MS 300

Tomodachi* tomo_alloc() {
    Tomodachi* tomodachi = malloc(sizeof(Tomodachi));
    Dolphin* dolphin = furi_record_open("dolphin");
    DolphinStats stats = dolphin_stats(dolphin);
    // Get tomodachi name from furi
    tomodachi->me.name = furi_hal_version_get_name_ptr();
    tomodachi->me.xp = stats.level;
    // Print debug info
    FURI_LOG_I(TAG, "Name: %s", tomodachi->me.name);
    FURI_LOG_I(TAG, "LV: %u", tomodachi->me.xp);
    return tomodachi;
}

bool tomo_send_message(Tomodachi* tomodachi, uint8_t* message, uint32_t length) {
    // Send message to all devices
    // Check TTL
    if(message[2] > 0) {
        // Substract 1 from TTL
        message[2]--;
        // Send message
        FURI_LOG_D(TAG, "Sending message: %s", message);
        subghz_tx_rx_worker_write(tomodachi->subghz_txrx, message, length);
    } else {
        FURI_LOG_D(TAG, "TTL expired");
        return false;
    }
    return true;
}

bool tomo_adv(Tomodachi* tomodachi) {
    // Advertize our info through subghz
    // Initialize network thing.
    uint32_t frequency = 433920000;
    size_t message_max_len = 180;
    UNUSED(message_max_len);
    tomodachi->subghz_txrx = subghz_tx_rx_worker_alloc();
    FURI_LOG_I(TAG, "Starting worker");
    subghz_tx_rx_worker_start(tomodachi->subghz_txrx, frequency);
    FURI_LOG_I(TAG, "Worker started");
    // Advertize our info
    // Compile message into one buffer.
    uint8_t message[180] = {0};
    message[TYPE] = 0x01; // Type: advertisement
    message[VERSION] = 0x00; // Version: 0.0.0
    message[TTL] = 0x08; // TTL: 8
    // Add timestamp
    FuriHalRtcDateTime date_time;
    furi_hal_rtc_get_datetime(&date_time);
    uint32_t timestamp = furi_hal_rtc_datetime_to_timestamp(&date_time);
    memcpy(message + 3, &timestamp, 4);
    size_t message_len = 3 + 4;
    for(int i = 0; i < 16; i++) {
        message[message_len++] = tomodachi->me.name[i];
    }
    message[message_len++] = tomodachi->me.xp >> 24;
    message[message_len++] = tomodachi->me.xp >> 16;
    message[message_len++] = tomodachi->me.xp >> 8;
    message[message_len++] = tomodachi->me.xp;
    // Send message 2 times.
    FURI_LOG_I(TAG, "Sending message");
    for(int i = 0; i < 2; i++) {
        tomo_send_message(tomodachi, message, message_len);
    }
    FURI_LOG_I(TAG, "Message sent");
    FURI_LOG_I(TAG, "Stopping worker");

    furi_hal_delay_ms(5);
    if(subghz_tx_rx_worker_is_running(tomodachi->subghz_txrx)) {
        subghz_tx_rx_worker_stop(tomodachi->subghz_txrx);
        subghz_tx_rx_worker_free(tomodachi->subghz_txrx);
    } else {
        furi_crash("Subghz worker is not running");
    }

    return true;
}

bool tomo_listen(Tomodachi* tomodachi, uint32_t timeout_ms) {
    UNUSED(tomodachi);
    // Listen for other devices advertising
    // Initialize network thing.
    uint32_t frequency = 433920000;
    size_t message_max_len = 180;
    tomodachi->subghz_txrx = subghz_tx_rx_worker_alloc();
    FURI_LOG_I(TAG, "Starting worker");
    subghz_tx_rx_worker_start(tomodachi->subghz_txrx, frequency);
    FURI_LOG_I(TAG, "Worker started");
    FURI_LOG_I(TAG, "Listening for messages");
    // Listen for other devices advertising
    // Wait for message
    size_t message_len = 0;
    uint8_t received_messages[2][180] = {0};
    while(timeout_ms > 0) {
        message_len = subghz_tx_rx_worker_read(
            tomodachi->subghz_txrx, received_messages[0], message_max_len);
        if(message_len > 0) {
            FURI_LOG_I(TAG, "Message: %s", received_messages[0]);
            // Check message
            if(message_len == 0) {
                FURI_LOG_I(TAG, "No message received");
                break;
            }
            if(received_messages[0][0] != 0x01) {
                FURI_LOG_I(TAG, "Invalid message type");
                break;
            }
            if(received_messages[0][1] != 0x00) {
                FURI_LOG_I(TAG, "Invalid message version");
                break;
            }

            FURI_LOG_W(TAG, "Message received");
            FURI_LOG_W(TAG, "TTL: %u", received_messages[0][2]);
            FURI_LOG_W(TAG, "Timestamp: %u", *(uint32_t*)(received_messages[0] + 3));

            FURI_LOG_W(TAG, "Name: %s", received_messages[0] + (3 + 4));
            FURI_LOG_W(
                TAG,
                "LV: %u",
                received_messages[0][19] << 24 | received_messages[0][20] << 16 |
                    received_messages[0][21] << 8 | received_messages[0][22]);
            for(int i = 0; i < 5; i++) {
                FURI_LOG_W(TAG, "%u", received_messages[0][23 + i]);
            }
        }
        furi_hal_delay_ms(1);
        timeout_ms -= 1;
    }

    if(subghz_tx_rx_worker_is_running(tomodachi->subghz_txrx)) {
        subghz_tx_rx_worker_stop(tomodachi->subghz_txrx);
        subghz_tx_rx_worker_free(tomodachi->subghz_txrx);
    } else {
        furi_crash("Subghz worker is not running");
    }
    FURI_LOG_I(TAG, "Message: %s", received_messages[0]);
    // Check message
    if(message_len == 0) {
        FURI_LOG_I(TAG, "No message received");
        return false;
    }
    if(received_messages[0][0] != 0x01) {
        FURI_LOG_I(TAG, "Invalid message type");
        return false;
    }
    if(received_messages[0][1] != 0x00) {
        FURI_LOG_I(TAG, "Invalid message version");
        return false;
    }

    FURI_LOG_W(TAG, "Message received");
    FURI_LOG_W(TAG, "Name: %s", received_messages[0] + 2);
    FURI_LOG_W(
        TAG,
        "LV: %u",
        received_messages[0][18] << 24 | received_messages[0][19] << 16 |
            received_messages[0][20] << 8 | received_messages[0][21]);
    return true;
}

uint32_t tomo_srv() {
    Tomodachi* tomodachi = tomo_alloc();
    // Loop forever, sending messages every 5 seconds
    FuriHalRtcDateTime date_time;
    while(1) {
        furi_hal_rtc_get_datetime(&date_time);
        if(date_time.minute % 5 == 0) {
            tomo_adv(tomodachi);
            furi_hal_delay_ms(1);
            tomo_listen(tomodachi, TOMODACHI_ADV_INTERVAL_MS + (rand() % 100));
        } else {
            furi_hal_delay_ms(5000);
            FURI_LOG_W(TAG, "Minutes: %u", date_time.minute);
            FURI_LOG_W(TAG, "Seconds: %u", date_time.second);
        }
    }
    furi_crash("Tomodachi service died");
    return 0;
}

uint32_t tomodachi_on_system_start(void* context) {
    UNUSED(context);
    return 0;
}