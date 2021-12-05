#include <furi.h>
#include <furi-hal.h>

#include <gui/gui.h>
#include <gui/icon.h>
#include <gui/icon_animation.h>
#include <input/input.h>
#include "assets_icons.h"

// TODO float note freq
typedef enum {
    // Delay
    N = 0,
    // Octave 4
    A4 = 440,
    B4 = 494,
    C4 = 262,
    D4 = 294,
    E4 = 330,
    G4 = 392,
    F4 = 349,
    // Octave 5
    C5 = 523,
    D5 = 587,
    E5 = 659,
    F5 = 740,
    F_5 = 740,
    G5 = 784,
    A5 = 880,
    B5 = 988,
    // Octave 6
    C6 = 1046,
    D6 = 1175,
    E6 = 1319,
} MelodyEventNote;

typedef enum {
    L1 = 1,
    L2 = 2,
    L4 = 4,
    L8 = 8,
    L16 = 16,
    L32 = 32,
    L64 = 64,
    L128 = 128,
} MelodyEventLength;

typedef struct {
    MelodyEventNote note;
    MelodyEventLength length;
} MelodyEventRecord;

typedef struct {
    const MelodyEventRecord* record;
    int8_t loop_count;
} SongPattern;

const MelodyEventRecord esx_start[] = {
    {A4, 17}, {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {A4, 17},
    {N, 17},  {A4, 17}, {A4, 17}, {G4, 17}, {A4, 17}, {N, 17},  {N, 17},  {A4, 17}, {N, 17},
    {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {A4, 17}, {N, 17},  {A4, 17},
    {A4, 17}, {G4, 17}, {A4, 17}, {N, 17},  {N, 17},  {A4, 17}, {N, 17},  {N, 17},  {N, 17},
    {N, 17},  {N, 17},  {C5, 17}, {N, 17},  {N, 17},  {N, 17},  {A4, 17}, {N, 17},  {N, 17},
    {N, 17},  {G4, 17}, {N, 17},  {G4, 17}, {N, 17},  {F4, 17}, {N, 17},  {N, 17},  {N, 17},
    {D4, 17}, {N, 17},  {D4, 17}, {N, 17},  {E4, 17}, {N, 17},  {F4, 17}, {N, 17},  {D4, 17},
    {N, 17},  {A4, 17}, {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},
    {A4, 17}, {N, 17},  {A4, 17}, {A4, 17}, {G4, 17}, {A4, 17}, {N, 17},  {N, 17},  {A4, 17},
    {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {A4, 17}, {N, 17},
    {A4, 17}, {A4, 17}, {G4, 17}, {A4, 17}, {N, 17},  {N, 17},  {A4, 17}, {N, 17},  {N, 17},
    {N, 17},  {N, 17},  {N, 17},  {C5, 17}, {N, 17},  {N, 17},  {N, 17},  {A4, 17}, {N, 17},
    {N, 17},  {N, 17},  {G4, 17}, {N, 17},  {G4, 17}, {N, 17},  {F4, 17}, {N, 17},  {N, 17},
    {N, 17},  {D4, 17}, {N, 17},  {D4, 17}, {N, 17},  {E4, 17}, {N, 17},  {F4, 17}, {N, 17},
    {D4, 17}, {N, 17}};

const MelodyEventRecord esx_loop[] = {
    {A4, 17}, {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {A4, 17},
    {N, 17},  {A4, 17}, {A4, 17}, {G4, 17}, {A4, 17}, {N, 17},  {N, 17},  {A4, 17}, {N, 17},
    {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {A4, 17}, {N, 17},  {A4, 17},
    {A4, 17}, {G4, 17}, {A4, 17}, {N, 17},  {N, 17},  {A4, 17}, {N, 17},  {N, 17},  {N, 17},
    {N, 17},  {N, 17},  {C5, 17}, {N, 17},  {N, 17},  {N, 17},  {A4, 17}, {N, 17},  {N, 17},
    {N, 17},  {G4, 17}, {N, 17},  {G4, 17}, {N, 17},  {F4, 17}, {N, 17},  {N, 17},  {N, 17},
    {D4, 17}, {N, 17},  {D4, 17}, {N, 17},  {E4, 17}, {N, 17},  {F4, 17}, {N, 17},  {D4, 17},
    {N, 17},  {A4, 17}, {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},
    {A4, 17}, {N, 17},  {A4, 17}, {A4, 17}, {G4, 17}, {A4, 17}, {N, 17},  {N, 17},  {A4, 17},
    {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {N, 17},  {A4, 17}, {N, 17},
    {A4, 17}, {A4, 17}, {G4, 17}, {A4, 17}, {N, 17},  {N, 17},  {A4, 17}, {N, 17},  {N, 17},
    {N, 17},  {N, 17},  {N, 17},  {C5, 17}, {N, 17},  {N, 17},  {N, 17},  {A4, 17}, {N, 17},
    {N, 17},  {N, 17},  {G4, 17}, {N, 17},  {G4, 17}, {N, 17},  {F4, 17}, {N, 17},  {N, 17},
    {N, 17},  {D4, 17}, {N, 17},  {D4, 17}, {N, 17},  {E4, 17}, {N, 17},  {F4, 17}, {N, 17},
    {D4, 17}, {N, 17}};

typedef enum {
    EventTypeTick,
    EventTypeKey,
    EventTypeNote,
    // add your events type
} MusicDemoEventType;

typedef struct {
    union {
        InputEvent input;
        const MelodyEventRecord* note_record;
    } value;
    MusicDemoEventType type;
} MusicDemoEvent;

typedef struct {
    ValueMutex* state_mutex;
    osMessageQueueId_t event_queue;

} MusicDemoContext;

#define note_stack_size 4
typedef struct {
    // describe state here
    const MelodyEventRecord* note_record;
    const MelodyEventRecord* note_stack[note_stack_size];
    uint8_t volume_id;
    uint8_t volume_id_max;
} State;

float esx_volumes[] = {0, 0.02, 0.05, 0.1, 0.5};

IconAnimation* ia;

static void esx_render_callback(Canvas* canvas, void* ctx) {
    //    State* state = (State*)acquire_mutex((ValueMutex*)ctx, 25);
    // static int cntr = 0;
    //    static

    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontPrimary);
    //    canvas_draw_str(canvas, 0, 12, "MusicPlayer");

    canvas_draw_icon_animation(canvas, 0, 0, ia);

    // release_mutex((ValueMutex*)ctx, state);
}

static void esx_input_callback(InputEvent* input_event, void* ctx) {
    osMessageQueueId_t event_queue = ctx;

    MusicDemoEvent event;
    event.type = EventTypeKey;
    event.value.input = *input_event;
    osMessageQueuePut(event_queue, &event, 0, 0);
}

void esx_process_note(
    const MelodyEventRecord* note_record,
    float bar_length_ms,
    MusicDemoContext* context) {
    MusicDemoEvent event;
    // send note event
    event.type = EventTypeNote;
    event.value.note_record = note_record;
    osMessageQueuePut(context->event_queue, &event, 0, 0);

    // read volume
    State* state = (State*)acquire_mutex(context->state_mutex, 25);
    float volume = esx_volumes[state->volume_id];
    release_mutex(context->state_mutex, state);

    // play note
    float note_delay = bar_length_ms / (float)note_record->length;
    if(note_record->note != N) {
        hal_pwm_set(volume, note_record->note, &SPEAKER_TIM, SPEAKER_CH);
    }
    delay(note_delay);
    hal_pwm_stop(&SPEAKER_TIM, SPEAKER_CH);
}

void esx_player_thread(void* p) {
    MusicDemoContext* context = (MusicDemoContext*)p;

    const float bpm = 130.0f;
    // 4/4
    const float bar_length_ms = (60.0f * 1000.0f / bpm) * 4;
    const uint16_t melody_start_events_count = sizeof(esx_start) / sizeof(esx_start[0]);
    const uint16_t melody_loop_events_count = sizeof(esx_loop) / sizeof(esx_loop[0]);

    for(size_t i = 0; i < melody_start_events_count; i++) {
        esx_process_note(&esx_start[i], bar_length_ms, context);
    }

    while(1) {
        for(size_t i = 0; i < melody_loop_events_count; i++) {
            esx_process_note(&esx_loop[i], bar_length_ms, context);
        }
    }
}

int32_t epic_sax_guy_app(void* p) {
    osMessageQueueId_t event_queue = osMessageQueueNew(8, sizeof(MusicDemoEvent), NULL);

    State _state;
    _state.note_record = NULL;
    for(size_t i = 0; i < note_stack_size; i++) {
        _state.note_stack[i] = NULL;
    }
    _state.volume_id = 1;
    _state.volume_id_max = sizeof(esx_volumes) / sizeof(esx_volumes[0]);

    ValueMutex state_mutex;
    if(!init_mutex(&state_mutex, &_state, sizeof(State))) {
        printf("cannot create mutex\r\n");
        return 255;
    }

    ia = icon_animation_alloc(&A_SaxGuy);
    icon_animation_start(ia);

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, esx_render_callback, &state_mutex);
    view_port_input_callback_set(view_port, esx_input_callback, event_queue);

    // Open GUI and register view_port
    Gui* gui = furi_record_open("gui");
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    // start player thread
    // TODO change to fuirac_start
    osThreadAttr_t player_attr = {.name = "esx_player_thread", .stack_size = 512};
    MusicDemoContext context = {.state_mutex = &state_mutex, .event_queue = event_queue};
    osThreadId_t player = osThreadNew(esx_player_thread, &context, &player_attr);

    if(player == NULL) {
        printf("cannot create player thread\r\n");
        return 255;
    }

    MusicDemoEvent event;
    while(1) {
        osStatus_t event_status = osMessageQueueGet(event_queue, &event, NULL, 100);

        State* state = (State*)acquire_mutex_block(&state_mutex);

        if(event_status == osOK) {
            if(event.type == EventTypeKey) {
                // press events
                if(event.value.input.type == InputTypeShort &&
                   event.value.input.key == InputKeyBack) {
                    release_mutex(&state_mutex, state);
                    break;
                }

                if(event.value.input.type == InputTypePress &&
                   event.value.input.key == InputKeyUp) {
                    if(state->volume_id < state->volume_id_max - 1) state->volume_id++;
                }

                if(event.value.input.type == InputTypePress &&
                   event.value.input.key == InputKeyDown) {
                    if(state->volume_id > 0) state->volume_id--;
                }

                if(event.value.input.type == InputTypePress &&
                   event.value.input.key == InputKeyLeft) {
                }

                if(event.value.input.type == InputTypePress &&
                   event.value.input.key == InputKeyRight) {
                }

                if(event.value.input.key == InputKeyOk) {
                }

            } else if(event.type == EventTypeNote) {
                state->note_record = event.value.note_record;

                for(size_t i = note_stack_size - 1; i > 0; i--) {
                    state->note_stack[i] = state->note_stack[i - 1];
                }
                state->note_stack[0] = state->note_record;
            }
        } else {
            // event timeout
        }

        view_port_update(view_port);
        release_mutex(&state_mutex, state);
    }

    icon_animation_stop(ia);
    icon_animation_free(ia);
    osThreadTerminate(player);
    hal_pwm_stop(&SPEAKER_TIM, SPEAKER_CH);
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close("gui");
    view_port_free(view_port);
    osMessageQueueDelete(event_queue);
    delete_mutex(&state_mutex);

    return 0;
}
