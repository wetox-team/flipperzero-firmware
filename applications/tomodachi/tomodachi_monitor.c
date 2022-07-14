
#include <furi.h>
#include <furi_hal.h>

#include <gui/gui.h>
#include <input/input.h>

#include <notification/notification_messages.h>

#include "tomodachi.h"
#include "tomodachi_monitor.h"

#define TAG "TomodachiMonitor"

static void tomodachi_app_draw_callback(Canvas* canvas, void* ctx) {
  Tomodachi* tomodachi = (Tomodachi*)ctx;
  // Draw tomodachi message's ttl
  char ttl_str[10] = {0};
  sprintf(ttl_str, "TTL: %d", tomodachi->last_message[TTL_POS]);
  canvas_draw_str(canvas, 0, 10, ttl_str);

  // Draw tomodachi message 
  char* message_str = malloc(tomodachi->last_message[LENGTH_POS] + 1);
  for (uint8_t i = 0; i < tomodachi->last_message[LENGTH_POS]; i++) {
    message_str[i] = tomodachi->last_message[i + PARAMS_LEN];
  }
  canvas_draw_str(canvas, 0, 20, message_str);

  FURI_LOG_W(TAG, "Message: %s", message_str);
  FURI_LOG_W(TAG, "TTL: %s", ttl_str);
  free(message_str);
}

static void tomodachi_app_input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    osMessageQueueId_t event_queue = ctx;

    TomoEvent event = {.type = TomoEventTypeInput, .input = *input_event};
    osMessageQueuePut(event_queue, &event, 0, osWaitForever);
}

void tomodachi_app_update(void* ctx) {
    furi_assert(ctx);
    osMessageQueueId_t event_queue = ctx;
    TomoEvent event = {.type = TomoEventTypeTick};
    osMessageQueuePut(event_queue, &event, 0, 0);
}

int32_t tomodachi_monitor(void* p) {
    UNUSED(p);
    // Register tomodachi data
    Tomodachi* tomodachi = furi_record_open("tomodachi");

    // Configure view port
    ViewPort* view_port = view_port_alloc();
    view_port_input_callback_set(view_port, tomodachi_app_input_callback, tomodachi);
    view_port_draw_callback_set(view_port, tomodachi_app_draw_callback, tomodachi);

    // Register view port in GUI
    Gui* gui = furi_record_open("gui");
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);
    view_port_update(view_port);


    while(1) { 
      view_port_update(view_port);
      furi_hal_delay_ms(100);
    }


    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);

    furi_record_close("gui");
    return 0;
}


