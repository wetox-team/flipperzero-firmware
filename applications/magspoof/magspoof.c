#include "magspoof.h"
#include "magspoof_i.h"

#define PIN_A 2
#define PIN_B 4
#define ENABLE_PIN 1
#define CLOCK_US 200

#define LINES_ON_SCREEN 6
#define COLUMNS_ON_SCREEN 21

uint8_t magspoof_bit_dir = 0;

typedef struct UartDumpModel UartDumpModel;

// typedef struct {
//     Gui* gui;
//     NotificationApp* notification;
//     ViewDispatcher* view_dispatcher;
//     View* view;
//     FuriThread* worker_thread;
//     StreamBufferHandle_t rx_stream;
// } Magspoof;

typedef struct {
    string_t text;
} ListElement;

struct UartDumpModel {
    ListElement* list[LINES_ON_SCREEN];
    uint8_t line;

    char last_char;
    bool escape;
};

typedef enum {
    WorkerEventReserved = (1 << 0), // Reserved for StreamBuffer internal event
    WorkerEventStop = (1 << 1),
    WorkerEventRx = (1 << 2),
} WorkerEventFlags;

#define WORKER_EVENTS_MASK (WorkerEventStop | WorkerEventRx)

const NotificationSequence magspoof_sequence_notification = {
    &message_display_on,
    &message_green_255,
    &message_delay_10,
    NULL,
};

static void magspoof_view_draw_callback(Canvas* canvas, void* _model) {
    UartDumpModel* model = _model;

    // Prepare canvas
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontKeyboard);

    for(size_t i = 0; i < LINES_ON_SCREEN; i++) {
        canvas_draw_str(
            canvas,
            0,
            (i + 1) * (canvas_current_font_height(canvas) - 1),
            string_get_cstr(model->list[i]->text));

        if(i == model->line) {
            uint8_t width = canvas_string_width(canvas, string_get_cstr(model->list[i]->text));

            canvas_draw_box(
                canvas,
                width,
                (i) * (canvas_current_font_height(canvas) - 1) + 2,
                2,
                canvas_current_font_height(canvas) - 2);
        }
    }
}


// Start spoof code
static void playBit(uint8_t sendBit)
{
  magspoof_bit_dir ^= 1;
  gpio_item_set_pin(PIN_A, magspoof_bit_dir);
  gpio_item_set_pin(PIN_B, !magspoof_bit_dir);
  delay_us(CLOCK_US);

  if (sendBit)
  {
    magspoof_bit_dir ^= 1;
    gpio_item_set_pin(PIN_A, magspoof_bit_dir);
    gpio_item_set_pin(PIN_B, !magspoof_bit_dir);
  }
  delay_us(CLOCK_US);
}

// static void magspoof_spoof(string_t track_str) {
static void magspoof_spoof() {
    // TODO
    // string_set_str(data, "\%qwe;test?");
    // track_str -> data
    char* data = ";123;test?\0";
    furi_hal_power_enable_otg();
    gpio_item_configure_all_pins(GpioModeOutputPushPull);
    delay(200);

    FURI_CRITICAL_ENTER();

    // gpio_item_set_pin(0, 1);
    const uint8_t track = 1;
    const uint8_t bitlen[] = {
        7, 5, 5 };
    const int sublen[] = {
        32, 48, 48 };
    int tmp, crc, lrc = 0;
    magspoof_bit_dir = 0;

    // enable H-bridge and LED
    gpio_item_set_pin(ENABLE_PIN, 1);

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

    gpio_item_set_pin(PIN_A, 0);
    gpio_item_set_pin(PIN_B, 0);
    gpio_item_set_pin(ENABLE_PIN, 0);

    FURI_CRITICAL_EXIT();

    delay(100);
    gpio_item_configure_all_pins(GpioModeAnalog);
    furi_hal_power_disable_otg();
}

// end my code

static bool magspoof_view_input_callback(InputEvent* event, void* context) {
    furi_assert(context);

    bool consumed = false;
    Magspoof* app = context;

    if(event->type == InputTypeShort) {
        if(event->key == InputKeyOk) {
            consumed = true;
            // furi_assert();
            // model->list;
            // string_reset(model->list[0]->text);
            
            // printf("%i", model->line);
            // for(size_t i = 0; i < LINES_ON_SCREEN; i++) {
            //     string_reset(model->list[i]->text);
            // }
            // model->line = 0;
            // model->escape = false;
            with_view_model(app->view, (UartDumpModel * model) {
                for(size_t i = 0; i < LINES_ON_SCREEN; i++) {
                    string_reset(model->list[i]->text);
                }
                model->line = 0;
                model->escape = false;
                return true;
            });
        }
        if(event->key == InputKeyLeft) {
            magspoof_spoof();
        }
    }
    return consumed;
}

static uint32_t magspoof_exit(void* context) {
    return VIEW_NONE;
}

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

static void magspoof_push_to_list(UartDumpModel* model, const char data) {
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
                                magspoof_push_to_list(model, data[i]);
                            }
                            return false;
                        });
                }
            } while(length > 0);

            notification_message(app->notification, &magspoof_sequence_notification);
            with_view_model(
                app->view, (UartDumpModel * model) { return true; });
        }
    }

    return 0;
}

static Magspoof* magspoof_alloc() {
    Magspoof* app = furi_alloc(sizeof(Magspoof));
    // UartDumpModel* cont = furi_alloc(sizeof(UartDumpModel));

    app->rx_stream = xStreamBufferCreate(2048, 1);

    // Gui
    app->gui = furi_record_open("gui");
    app->notification = furi_record_open("notification");

    // View dispatcher
    app->view_dispatcher = view_dispatcher_alloc();
    app->scene_manager = scene_manager_alloc(&magspoof_scene_handlers, app);
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // Views
    app->view = view_alloc();
    view_set_draw_callback(app->view, magspoof_view_draw_callback);
    view_set_input_callback(app->view, magspoof_view_input_callback);
    view_allocate_model(app->view, ViewModelTypeLocking, sizeof(UartDumpModel));

    view_set_context(app->view, app);

    with_view_model(
        app->view, (UartDumpModel * model) {
            for(size_t i = 0; i < LINES_ON_SCREEN; i++) {
                model->line = 0;
                model->escape = false;
                model->list[i] = furi_alloc(sizeof(ListElement));
                string_init(model->list[i]->text);
            }
            return true;
        });


    // Submenu
    app->submenu = submenu_alloc();
    view_dispatcher_add_view(app->view_dispatcher, MagspoofViewMenu, submenu_get_view(app->submenu));


    view_set_previous_callback(app->view, magspoof_exit);

    // view_dispatcher_add_view(app->view_dispatcher, 0, app->view);
    view_dispatcher_switch_to_view(app->view_dispatcher, 0);

    // Enable uart listener
    furi_hal_console_disable();
    furi_hal_uart_set_br(FuriHalUartIdUSART1, 9600);
    furi_hal_uart_set_irq_cb(FuriHalUartIdUSART1, magspoof_on_irq_cb, app);

    app->worker_thread = furi_thread_alloc();
    furi_thread_set_name(app->worker_thread, "MagspoofWorker");
    furi_thread_set_stack_size(app->worker_thread, 1024);
    furi_thread_set_context(app->worker_thread, app);
    furi_thread_set_callback(app->worker_thread, magspoof_worker);
    furi_thread_start(app->worker_thread);

    return app;
}

static void magspoof_free(Magspoof* app) {
    furi_assert(app);

    osThreadFlagsSet(furi_thread_get_thread_id(app->worker_thread), WorkerEventStop);
    furi_thread_join(app->worker_thread);
    furi_thread_free(app->worker_thread);

    furi_hal_console_enable();

    // Free views
    view_dispatcher_remove_view(app->view_dispatcher, 0);

    with_view_model(
        app->view, (UartDumpModel * model) {
            for(size_t i = 0; i < LINES_ON_SCREEN; i++) {
                string_clear(model->list[i]->text);
                free(model->list[i]);
            }
            return true;
        });
    view_free(app->view);
    view_dispatcher_free(app->view_dispatcher);

    scene_manager_free(app->scene_manager);

    // Close gui record
    furi_record_close("gui");
    furi_record_close("notification");
    app->gui = NULL;

    vStreamBufferDelete(app->rx_stream);

    // Free rest
    free(app);
}

int32_t magspoof_app(void* p) {
    Magspoof* app = magspoof_alloc();

    scene_manager_next_scene(app->scene_manager, MagspoofSceneStart);

    view_dispatcher_run(app->view_dispatcher);
    magspoof_free(app);
    return 0;
}