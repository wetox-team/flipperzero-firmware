
#include <furi.h>
#include <furi_hal.h>

#include <gui/gui.h>
#include <input/input.h>

#include <notification/notification_messages.h>

#include "tomodachi.h"
#include "tomodachi_monitor.h"

#define TAG "TomodachiMonitor"

static const NotificationSequence tomodachi_sequence_rx = {
    &message_green_255,

    &message_vibro_on,
    &message_note_c6,
    &message_delay_50,
    &message_sound_off,
    &message_vibro_off,

    &message_delay_50,
    NULL,
};

static void tomodachi_app_draw_callback(Canvas* canvas, void* ctx) {
    Tomodachi* tomodachi = (Tomodachi*)ctx;
    // Draw tomodachi message's ttl
    char ttl_str[10] = {0};
    sprintf(ttl_str, "TTL: %d", tomodachi->last_message[TTL_POS]);
    canvas_draw_str(canvas, 0, 10, ttl_str);

    // Draw tomodachi message
    char* message_str = malloc(tomodachi->last_message[LENGTH_POS] + 1);
    for(uint8_t i = 0; i < tomodachi->last_message[LENGTH_POS]; i++) {
        message_str[i] = tomodachi->last_message[i + PARAMS_LEN];
    }
    canvas_draw_str(canvas, 0, 20, message_str);
    free(message_str);
    // Draw tomodachi timestamp
    uint8_t* timestamp = malloc(4); // bytes 16-19 of message
    for(uint8_t i = 0; i < 4; i++) {
        timestamp[i] = tomodachi->last_message[i + 16];
    }
    char timestamp_str[20] = {0};
    sprintf(timestamp_str, "Timestamp: %ld", (uint32_t)timestamp);
    canvas_draw_str(canvas, 0, 30, timestamp_str);
    free(timestamp);
    // Print known devices count
    char known_devices_str[20] = {0};
    sprintf(known_devices_str, "Known devices: %d", tomodachi->tomodachi_known_count);
    canvas_draw_str(canvas, 0, 40, known_devices_str);

    // Handle beeping
    // Compare prev and last_message
    if(memcmp(tomodachi->last_message, tomodachi->prev_message, sizeof(tomodachi->last_message)) !=
       0) {
        // Blink an LED and vibrate
        notification_message(tomodachi->notifications, &tomodachi_sequence_rx);
        // Update previous message
        memcpy(tomodachi->prev_message, tomodachi->last_message, sizeof(tomodachi->last_message));
    }
}

static void tomodachi_app_input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;

    TomoEvent event = {.type = TomoEventTypeInput, .input = *input_event};
    furi_message_queue_put(event_queue, &event, FuriWaitForever);
}

void tomodachi_app_update(void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;
    TomoEvent event = {.type = TomoEventTypeTick};
    furi_message_queue_put(event_queue, &event, 0);
}

int32_t tomodachi_monitor(void* p) {
    UNUSED(p);
    // Register tomodachi data
    Tomodachi* tomodachi = furi_record_open("tomodachi");

    // Create event queue
    //osMessageQueueId_t event_queue = osMessageQueueNew(8, sizeof(TomoEvent), NULL);
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(TomoEvent));

    // Configure view port
    ViewPort* view_port = view_port_alloc();
    view_port_input_callback_set(view_port, tomodachi_app_input_callback, event_queue);
    view_port_draw_callback_set(view_port, tomodachi_app_draw_callback, tomodachi);

    // Register view port in GUI
    Gui* gui = furi_record_open("gui");
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);
    view_port_update(view_port);

    // Open notification record
    tomodachi->notifications = furi_record_open("notification");

    TomoEvent event;

    while(1) {
        view_port_update(view_port);
        furi_delay_ms(100);

        furi_check(furi_message_queue_get(event_queue, &event, FuriWaitForever) == FuriStatusOk);

        if((event.input.type == InputTypeShort) && (event.input.key == InputKeyBack)) {
            break;
        }
    }

    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);

    furi_record_close("gui");
    furi_record_close("tomodachi");
    furi_record_close("notification");
    return 0;
}
