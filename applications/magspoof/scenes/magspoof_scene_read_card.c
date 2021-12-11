#include "../magspoof_i.h"
#include <furi-hal-resources.h>

#define MAGSPOOF_READ_CARD_CUSTOM_EVENT (10UL)
#define MAGSPOOF_READ_CARD_DRAW_EVENT (999)

#define WORKER_EVENTS_MASK (WorkerEventStop | WorkerEventRx)

const NotificationSequence magspoof_sequence_notification = {
    &message_display_on,
    &message_green_255,
    &message_delay_10,
    NULL,
};


#define PIN_A 2
#define PIN_B 4
#define ENABLE_PIN 1
#define CLOCK_US 500

uint8_t magspoof_bit_dir = 0;

void magspoof_scene_read_card_dialog_callback(DialogExResult result, void* context) {
    Magspoof* magspoof = (Magspoof*)context;

    view_dispatcher_send_custom_event(magspoof->view_dispatcher, result);
}


// static void magspoof_view_draw_callback(Canvas* canvas, void* _model) {
//     UartDumpModel* model = _model;

//     // Prepare canvas
//     canvas_clear(canvas);
//     canvas_set_color(canvas, ColorBlack);
//     canvas_set_font(canvas, FontKeyboard);

//     for(size_t i = 0; i < LINES_ON_SCREEN; i++) {
//         canvas_draw_str(
//             canvas,
//             0,
//             (i + 1) * (canvas_current_font_height(canvas) - 1),
//             string_get_cstr(model->list[i]->text));

//         if(i == model->line) {
//             uint8_t width = canvas_string_width(canvas, string_get_cstr(model->list[i]->text));

//             canvas_draw_box(
//                 canvas,
//                 width,
//                 (i) * (canvas_current_font_height(canvas) - 1) + 2,
//                 2,
//                 canvas_current_font_height(canvas) - 2);
//         }
//     }
// }

// Start spoof code

void gpio_item_set_rfid_pin(uint8_t index, bool level) {
    // furi_assert(index < GPIO_ITEM_COUNT);
    if (index == 2) {
        // hal_gpio_write(&gpio_ext_pa2, level);
        // hal_gpio_write(&gpio_rfid_pull, level);
        hal_gpio_write(&gpio_rfid_carrier_out, level);
    }
    if (index == 1) {
        // hal_gpio_write(&gpio_ext_pb13, level);
        
        // hal_gpio_write(&gpio_ext_pa7, level);
    }
}

static void playBit(uint8_t sendBit)
{
  magspoof_bit_dir ^= 1;
  gpio_item_set_rfid_pin(PIN_A, magspoof_bit_dir);
  gpio_item_set_rfid_pin(PIN_B, !magspoof_bit_dir);
  delay_us(CLOCK_US);

  if (sendBit)
  {
    magspoof_bit_dir ^= 1;
    gpio_item_set_rfid_pin(PIN_A, magspoof_bit_dir);
    gpio_item_set_rfid_pin(PIN_B, !magspoof_bit_dir);
  }
  delay_us(CLOCK_US);
}

static void magspoof_spoof(string_t track_str, uint8_t track) {
    // TODO
    // string_set_str(data, "\%qwe;test?");
    // track_str -> data
    // char* data = "%B123456781234567^LASTNAME/FIRST^YYMMSSSDDDDDDDDDDDDDDDDDDDDDDDDD?";
    // char* data = ";123456781234567=YYMMSSSDDDDDDDDDDDDDD?\0";
    
    furi_hal_power_enable_otg();

    size_t from;
    size_t to;

    // TODO ';' in first track case
    if (track == 0) {
        from = string_search_char(track_str, '%');
        to = string_search_char(track_str, '?', from);
    } else if (track == 1) {
        from = string_search_char(track_str, ';');
        to = string_search_char(track_str, '?', from);
    } else {
        from = 0;
        to = string_size(track_str);
    }
    if (from >= to) {
        return;
    }
    string_mid(track_str, from, to-from +1);

    const char* data = string_get_cstr(track_str);

    printf("%s",data);
    
    // gpio_item_configure_all_pins(GpioModeOutputPushPull);

    furi_hal_ibutton_start();
    furi_hal_ibutton_pin_low();
    
    // hal_gpio_init(&gpio_rfid_carrier, GpioModeOutputPushPull, GpioPullNo, GpioSpeedLow);
    
    
    hal_gpio_init(&gpio_rfid_pull, GpioModeOutputPushPull, GpioPullNo, GpioSpeedLow);
    hal_gpio_write(&gpio_rfid_pull, false);

    hal_gpio_init(&gpio_rfid_carrier_out, GpioModeOutputPushPull, GpioPullNo, GpioSpeedLow);
    
    delay(300);

    FURI_CRITICAL_ENTER();

    // TRECK NUM +1
    // const uint8_t track = 0;

    const uint8_t bitlen[] = {
        7, 5, 5 };
    const int sublen[] = {
        32, 48, 48 };
    int tmp, crc, lrc = 0;
    magspoof_bit_dir = 0;

    // enable H-bridge and LED
    gpio_item_set_rfid_pin(ENABLE_PIN, 1);

    // First put out a bunch of leading zeros.
    for (uint8_t i = 0; i < 25; i++) {
        playBit(0);
    }

    //
    for (uint8_t i = 0; data[i] != '\0'; i++)
    {
        crc = 1;
        tmp = data[i] - sublen[track];

        for (uint8_t j = 0; j < bitlen[track]-1; j++)
        {
            crc ^= tmp & 1;
            lrc ^= (tmp & 1) << j;
            playBit(tmp & 1);
            tmp >>= 1;
        }
        playBit(crc);
    }

    // finish calculating and send last "byte" (LRC)
    tmp = lrc;
    crc = 1;
    for (uint8_t j = 0; j < bitlen[track]-1; j++)
    {
        crc ^= tmp & 1;
        playBit(tmp & 1);
        tmp >>= 1;
    }
    playBit(crc);

    // finish with 0's
    for (uint8_t i = 0; i < 5 * 5; i++) {
        playBit(0);
    }

    gpio_item_set_rfid_pin(PIN_A, 0);
    gpio_item_set_rfid_pin(PIN_B, 0);
    gpio_item_set_rfid_pin(ENABLE_PIN, 0);

    FURI_CRITICAL_EXIT();

    furi_hal_rfid_pins_reset();

    // gpio_item_configure_all_pins(GpioModeAnalog);
    furi_hal_power_disable_otg();
}

// end my code

static void magspoof_clear_list(void* context) {
    Magspoof* app = context;
    string_reset(app->dev->data);
    // with_view_model(app->view, (UartDumpModel * model) {
    //             for(size_t i = 0; i < LINES_ON_SCREEN; i++) {
    //                 string_reset(model->list[i]->text);
    //             }
    //             model->line = 0;
    //             model->escape = false;
    //             return true;
    //         });
}

// static bool magspoof_view_input_callback(InputEvent* event, void* context) {
//     furi_assert(context);

//     bool consumed = false;
//     Magspoof* app = context;

//     if(event->type == InputTypeShort) {
//         if(event->key == InputKeyOk) {
//             consumed = true;
//             // furi_assert();
//             // model->list;
//             // string_reset(model->list[0]->text);
            
//             // printf("%i", model->line);
//             // for(size_t i = 0; i < LINES_ON_SCREEN; i++) {
//             //     string_reset(model->list[i]->text);
//             // }
//             // model->line = 0;
//             // model->escape = false;
//             magspoof_clear_list(app);
//         }
//         if(event->key == InputKeyLeft) {
//             string_t v;
//             string_init(v);
//             string_set_str(v,";123456781234567=YYMMSSSDDDDDDDDDDDDDD?")
//             magspoof_spoof(v,1);
//         }
//     }
//     return consumed;
// }


static void magspoof_on_irq_cb(UartIrqEvent ev, uint8_t data, void* context) {
    furi_assert(context);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    Magspoof* app = context;

    if(ev == UartIrqEventRXNE) {
        xStreamBufferSendFromISR(app->rx_stream, &data, 1, &xHigherPriorityTaskWoken);
        osThreadFlagsSet(furi_thread_get_thread_id(app->worker_thread), WorkerEventRx);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

static void magspoof_push_to_list(UartDumpModel* model, const char data, void* context) {
    Magspoof* app = context;
    
    string_push_back(app->dev->data, data);

    if(model->escape) {
        // escape code end with letter
        if((data >= 'a' && data <= 'z') || (data >= 'A' && data <= 'Z')) {
            model->escape = false;
        }
    } else if(data == '[' && model->last_char == '\e') {
        // "Esc[" is a escape code
        model->escape = true;
    } else if((data >= ' ' && data <= '~') || (data == '\n' || data == '\r')) {
        bool new_string_needed = false;
        if(string_size(model->list[model->line]->text) >= COLUMNS_ON_SCREEN) {
            new_string_needed = true;
        } else if((data == '\n' || data == '\r')) {
            // pack line breaks
            if(model->last_char != '\n' && model->last_char != '\r') {
                new_string_needed = true;
            }
        }

        if(new_string_needed) {
            if((model->line + 1) < LINES_ON_SCREEN) {
                model->line += 1;
            } else {
                ListElement* first = model->list[0];

                for(size_t i = 1; i < LINES_ON_SCREEN; i++) {
                    model->list[i - 1] = model->list[i];
                }

                string_reset(first->text);
                model->list[model->line] = first;
            }
        }

        if(data != '\n' && data != '\r') {
            string_push_back(model->list[model->line]->text, data);
        }
    }
    model->last_char = data;
}

static int32_t magspoof_worker(void* context) {
    furi_assert(context);
    Magspoof* app = context;

    while(1) {
        uint32_t events = osThreadFlagsWait(WORKER_EVENTS_MASK, osFlagsWaitAny, osWaitForever);
        furi_check((events & osFlagsError) == 0);

        if(events & WorkerEventStop) break;
        if(events & WorkerEventRx) {
            size_t length = 0;
            do {
                uint8_t data[64];
                length = xStreamBufferReceive(app->rx_stream, data, 64, 0);
                if(length > 0) {
                    furi_hal_uart_tx(FuriHalUartIdUSART1, data, length);
                    with_view_model(
                        app->view, (UartDumpModel * model) {
                            for(size_t i = 0; i < length; i++) {
                                magspoof_push_to_list(model, data[i], app);
                            }
                            return false;
                        });
                    
                }
            } while(length > 0);

            notification_message(app->notification, &magspoof_sequence_notification);
            with_view_model(
                app->view, (UartDumpModel * model) { return true; });
            
            
            // XXX make event
            view_dispatcher_send_custom_event(app->view_dispatcher, MAGSPOOF_READ_CARD_DRAW_EVENT);
        }
    }

    return 0;
}

// void magspoof_read_card_worker_callback(void* context) {
//     Magspoof* app = (Magspoof*)context;
//     view_dispatcher_send_custom_event(app->view_dispatcher, MAGSPOOF_READ_CARD_CUSTOM_EVENT);
// }

void magspoof_scene_read_card_on_enter(void* context) {
    Magspoof* app = (Magspoof*)context;

    // // Setup view
    // Popup* popup = nfc->popup;
    // popup_set_header(popup, "Detecting\nmag card", 70, 34, AlignLeft, AlignTop);
    // popup_set_icon(popup, 0, 3, &I_RFIDDolphinReceive_97x61);

    // Start worker
    // view_dispatcher_switch_to_view(app->view_dispatcher, NfcViewPopup);

    // magspoof_worker_start(
    //     app->worker, NfcWorkerStateDetect, &app->dev->dev_data, app_read_card_worker_callback, app);

    // view_set_draw_callback(app->view, magspoof_view_draw_callback);
    // view_set_input_callback(app->view, magspoof_view_input_callback);


    // Setup view
    // XXX
    // MagspoofDeviceCommonData* data = &app->dev->dev_data.magspoof_data;

    DialogEx* dialog_ex = app->dialog_ex;
    dialog_ex_set_left_button_text(dialog_ex, "Emulate");
    dialog_ex_set_right_button_text(dialog_ex, "Save");
    dialog_ex_set_center_button_text(dialog_ex, "Clear");

    // XXX TODO: fill data

    dialog_ex_set_text(dialog_ex, app->text_store, 0, 0, AlignLeft, AlignTop);
    dialog_ex_set_context(dialog_ex, app);
    dialog_ex_set_result_callback(dialog_ex, magspoof_scene_read_card_dialog_callback);

    view_dispatcher_switch_to_view(app->view_dispatcher, MagspoofViewDialogEx);

    view_dispatcher_send_custom_event(app->view_dispatcher, MAGSPOOF_READ_CARD_DRAW_EVENT);

    // Worker
    app->worker_thread = furi_thread_alloc();
    furi_thread_set_name(app->worker_thread, "MagspoofWorker");
    furi_thread_set_stack_size(app->worker_thread, 1024);
    furi_thread_set_context(app->worker_thread, app);
    furi_thread_set_callback(app->worker_thread, magspoof_worker);
    furi_thread_start(app->worker_thread);

    // UART
    furi_hal_uart_set_br(FuriHalUartIdUSART1, 9600);
    furi_hal_uart_set_irq_cb(FuriHalUartIdUSART1, magspoof_on_irq_cb, app);

}

bool magspoof_scene_read_card_on_event(void* context, SceneManagerEvent event) {
    Magspoof* app = (Magspoof*)context;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == MAGSPOOF_READ_CARD_DRAW_EVENT) {
            int8_t COUNT = 4;
            string_t res;
            string_init(res);
            for (int8_t i = 0; i < string_size(app->dev->data); i+=1) {
                // string_t tmp;
                // string_init_set(tmp, app->dev->data);
                // string_mid(tmp, i, COUNT);
                // // dialog_ex_set_text(app->dialog_ex, string_get_cstr( tmp ), 0, 0, AlignLeft, AlignTop);
                // string_sets()
                string_push_back(res, string_get_char(app->dev->data, i));
                if (i % COUNT == 0) {
                    string_push_back(res, ' ');
                }
            }
            dialog_ex_set_text(app->dialog_ex, string_get_cstr( res ), 0, 0, AlignLeft, AlignTop);
            return true;
        }
        // else if(event.event == MAGSPOOF_READ_CARD_CUSTOM_EVENT) {
        //     // scene_manager_next_scene(app->scene_manager, NfcSceneReadCardSuccess);
        //     string_t v;
        //     string_init(v);
        //     string_set_str(v,";123456781234567=YYMMSSSDDDDDDDDDDDDDD?");
        //     magspoof_spoof(v,1);


        //     magspoof_clear_list(app);
        //     return true;
        // }
    }
    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == DialogExResultLeft) {
            string_t v;
            string_init(v);
            string_set_str(v,"%B123456781234567^LASTNAME/FIRST^YYMMSSSDDDDDDDDDDDDDDDDDDDDDDDDD?;1234567812?");
            // string_set_str(v,"%B123456781234567^LASTNAME/FIRST^YYMMSSSDDDDDDDDDDDDDDDDDDDDDDDDD?");
            magspoof_spoof(v, 0);
            string_clear(v);


            // string_t v1;
            // string_t v2;
            // string_init_set(v1, app->dev->data);
            // string_init_set(v2, v1);
            // magspoof_spoof(v1, 0);
            // delay(500);
            // magspoof_spoof(v2, 1);

            // string_clear(v1);
            // string_clear(v2);
            
            return true;
        } else if(event.event == DialogExResultCenter) {
            magspoof_clear_list(app);
            view_dispatcher_send_custom_event(app->view_dispatcher, MAGSPOOF_READ_CARD_DRAW_EVENT);
            return true;
        } else if(event.event == DialogExResultRight) {
            // Clear device name
            // nfc_device_set_name(nfc->dev, "");
            // scene_manager_next_scene(nfc->scene_manager, NfcSceneCardMenu);
            // magspoof_device_save(app->dev, "Test");
            char * d = furi_alloc(1);
            d[0] = string_get_char(app->dev->data, 2);
            magspoof_device_save(app->dev, d);
            return true;
        } 
        // else if(event.event == DialogExResultUp) {
        //     // Clear device name
        //     // nfc_device_set_name(nfc->dev, "");
        //     // scene_manager_next_scene(nfc->scene_manager, NfcSceneCardMenu);
        //     magspoof_device_delete(app->dev, "Test");
        //     return true;
        // }
    }
    // else if(event.type == SceneManagerEventTypeTick) {
    //     notification_message(app->notification, &sequence_blink_blue_10);
    //     return true;
    // }
    return false;
}

void magspoof_scene_read_card_on_exit(void* context) {
    Magspoof* app = (Magspoof*)context;

    // Stop worker
    // magspoof_worker_stop(nfc->worker);
    osThreadFlagsSet(furi_thread_get_thread_id(app->worker_thread), WorkerEventStop);
    furi_thread_join(app->worker_thread);
    furi_thread_free(app->worker_thread);

    // Clear view
    // Popup* popup = nfc->popup;
    // popup_set_header(popup, NULL, 0, 0, AlignCenter, AlignBottom);
    // popup_set_text(popup, NULL, 0, 0, AlignCenter, AlignTop);
    // popup_set_icon(popup, 0, 0, NULL);
}
