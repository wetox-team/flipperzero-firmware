#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>

#include "contants.h"

typedef struct {
    //    +-----x
    //    |
    //    |
    //    y
    uint8_t x;
    uint8_t y;
} Point;

typedef enum {
    GameStateLife,
    GameStateLastChance,
    GameStateGameOver,
} GameState;

typedef enum {
    DirectionUp,
    DirectionRight,
    DirectionDown,
    DirectionLeft,
} Direction;

typedef enum {
    ModeSingle,
    ModeCooperative,
} Mode;

typedef struct {
    Point coordinates;
    Direction direction;
} ProjectileState;

typedef struct {
    // char map[FIELD_WIDTH][FIELD_HEIGHT];
    char map[16][11];
    Mode mode;
    GameState state;
    ProjectileState *projectiles[100];
    PlayerState p1;
    PlayerState p2;
} TanksState;

typedef struct {
    Point coordinates;
    uint16_t score;
    uint8_t lives;
    Direction direction;
    bool moving;
    uint8_t cooldown;
} PlayerState;

typedef enum {
    EventTypeTick,
    EventTypeKey,
} EventType;

typedef struct {
    EventType type;
    InputEvent input;
} TanksEvent;

static void tanks_game_render_callback(Canvas* const canvas, void* ctx) {
    const TanksState* tanks_state = acquire_mutex((ValueMutex*)ctx, 25);
    if(tanks_state == NULL) {
        return;
    }

    // Before the function is called, the state is set with the canvas_reset(canvas)

    canvas_draw_box(canvas, FIELD_WIDTH * CELL_LENGTH_PIXELS, 0, 2, SCREEN_HEIGHT);

    // Player
    Point coordinates = tanks_state->coordinates;

    switch (tanks_state->direction) {
        case DirectionUp:
            canvas_draw_icon(canvas, coordinates.x * CELL_LENGTH_PIXELS, coordinates.y * CELL_LENGTH_PIXELS - 1, &I_tank_up);
            break;
        case DirectionDown:
            canvas_draw_icon(canvas, coordinates.x * CELL_LENGTH_PIXELS, coordinates.y * CELL_LENGTH_PIXELS - 1, &I_tank_down);
            break;
        case DirectionRight:
            canvas_draw_icon(canvas, coordinates.x * CELL_LENGTH_PIXELS, coordinates.y * CELL_LENGTH_PIXELS - 1, &I_tank_right);
            break;
        case DirectionLeft:
            canvas_draw_icon(canvas, coordinates.x * CELL_LENGTH_PIXELS, coordinates.y * CELL_LENGTH_PIXELS - 1, &I_tank_left);
            break;
    }

    for(int8_t x = 0; x < FIELD_WIDTH; x++) {
        for(int8_t y = 0; y < FIELD_HEIGHT; y++) {
            switch (tanks_state->map[x][y]) {
            case '-':
                canvas_draw_icon(canvas, x * CELL_LENGTH_PIXELS, y * CELL_LENGTH_PIXELS - 1, &I_tank_wall);
                break;

            case '=':
                canvas_draw_icon(canvas, x * CELL_LENGTH_PIXELS, y * CELL_LENGTH_PIXELS - 1, &I_tank_stone);
                break;

            case '*':
                canvas_draw_icon(canvas, x * CELL_LENGTH_PIXELS, y * CELL_LENGTH_PIXELS - 1, &I_tank_hedgehog);
                break;

            case 'a':
                canvas_draw_icon(canvas, x * CELL_LENGTH_PIXELS, y * CELL_LENGTH_PIXELS - 1, &I_tank_base);
                break;
            }
        }
    }

    // Game Over banner
    if(tanks_state->state == GameStateGameOver) {
        // Screen is 128x64 px
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, 34, 20, 62, 24);

        canvas_set_color(canvas, ColorBlack);
        canvas_draw_frame(canvas, 34, 20, 62, 24);

        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 37, 31, "Game Over");

        canvas_set_font(canvas, FontSecondary);
        char buffer[13];
        snprintf(buffer, sizeof(buffer), "Score: %u", tanks_state->score);
        canvas_draw_str_aligned(canvas, 64, 41, AlignCenter, AlignBottom, buffer);
    }

    release_mutex((ValueMutex*)ctx, tanks_state);
}

static void tanks_game_input_callback(InputEvent* input_event, osMessageQueueId_t event_queue) {
    furi_assert(event_queue);

    TanksEvent event = {.type = EventTypeKey, .input = *input_event};
    osMessageQueuePut(event_queue, &event, 0, osWaitForever);
}

static void tanks_game_update_timer_callback(osMessageQueueId_t event_queue) {
    furi_assert(event_queue);

    TanksEvent event = {.type = EventTypeTick};
    osMessageQueuePut(event_queue, &event, 0, 0);
}

static void tanks_game_init_game(TanksState* const tanks_state) {
    //char map[FIELD_HEIGHT][FIELD_WIDTH + 1] = {
    char map[11][16 + 1] = {
        "*       -  *   -",
        "  -  -  =       ",
        "        -  -   2",
        "1    =     - -- ",
        "--   =     - -- ",
        "a-   =  -  =   2",
        "--   =     - -- ",
        "1    =     - -- ",
        "        -  -   2",
        "  -  -  =       ",
        "*       -  *   -",
    };

    Point c = {5, 5};

    for(int8_t x = 0; x < FIELD_WIDTH; x++) {
        for(int8_t y = 0; y < FIELD_HEIGHT; y++) {
            tanks_state->map[x][y] = ' ';

            if (map[y][x] == '1') {
                c.x = x;
                c.y = y;
            }

            if (map[y][x] == '-') {
                tanks_state->map[x][y] = '-';
            }

            if (map[y][x] == '=') {
                tanks_state->map[x][y] = '=';
            }

            if (map[y][x] == '*') {
                tanks_state->map[x][y] = '*';
            }

            if (map[y][x] == 'a') {
                tanks_state->map[x][y] = 'a';
            }
        }
    }

    tanks_state->coordinates = c;
    tanks_state->score = 0;
    tanks_state->cooldown = SHOT_COOLDOWN;
    tanks_state->moving = false;
    tanks_state->direction = DirectionUp;
    tanks_state->state = GameStateLife;
}

static bool tanks_game_collision(Point const next_step, TanksState const* const tanks_state) {
    // if x == 0 && currentMovement == left then x - 1 == 255 ,
    // so check only x > right border

    if (next_step.x < 0 || next_step.y < 0) {
        return true;
    }

    if (next_step.x >= FIELD_WIDTH || next_step.y >= FIELD_HEIGHT) {
        return true;
    }

    char tile = tanks_state->map[next_step.x][next_step.y];

    if (tile == '-' || tile == '=' || tile == '*' || tile == 'a') {
        return true;
    }

    return false;
}

static Point tanks_game_get_next_step(TanksState const* const tanks_state) {
    Point next_step = tanks_state->coordinates;
    switch(tanks_state->direction) {
        // +-----x
        // |
        // |
        // y
        case DirectionUp:
            next_step.y--;
            break;
        case DirectionRight:
            next_step.x++;
            break;
        case DirectionDown:
            next_step.y++;
            break;
        case DirectionLeft:
            next_step.x--;
            break;
    }
    return next_step;
}

static void tanks_game_move(TanksState* const tanks_state, Point const next_step) {
    tanks_state->coordinates = next_step;
}

static void tanks_game_process_game_step(TanksState* const tanks_state) {
    if(tanks_state->state == GameStateGameOver) {
        return;
    }

    if(tanks_state->moving) {
        Point next_step = tanks_game_get_next_step(tanks_state);
        bool crush = tanks_game_collision(next_step, tanks_state);

        if(!crush) {
            tanks_game_move(tanks_state, next_step);
        }
    }

    tanks_state->moving = false;
}

int32_t tanks_game_app(void* p) {
    srand(DWT->CYCCNT);

    osMessageQueueId_t event_queue = osMessageQueueNew(8, sizeof(TanksEvent), NULL);

    TanksState* tanks_state = furi_alloc(sizeof(TanksState));
    tanks_game_init_game(tanks_state);

    ValueMutex state_mutex;
    if(!init_mutex(&state_mutex, tanks_state, sizeof(TanksState))) {
        furi_log_print(FURI_LOG_ERROR, "cannot create mutex\r\n");
        free(tanks_state);
        return 255;
    }

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, tanks_game_render_callback, &state_mutex);
    view_port_input_callback_set(view_port, tanks_game_input_callback, event_queue);

    osTimerId_t timer =
            osTimerNew(tanks_game_update_timer_callback, osTimerPeriodic, event_queue, NULL);
    osTimerStart(timer, osKernelGetTickFreq() / 4);

    // Open GUI and register view_port
    Gui* gui = furi_record_open("gui");
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    TanksEvent event;
    for(bool processing = true; processing;) {
        osStatus_t event_status = osMessageQueueGet(event_queue, &event, NULL, 100);

        TanksState* tanks_state = (TanksState*)acquire_mutex_block(&state_mutex);

        if(event_status == osOK) {
            // press events
            if(event.type == EventTypeKey) {
                if(event.input.type == InputTypePress) {
                    switch(event.input.key) {
                        case InputKeyUp:
                            tanks_state->moving = true;
                            tanks_state->direction = DirectionUp;
                            break;
                        case InputKeyDown:
                            tanks_state->moving = true;
                            tanks_state->direction = DirectionDown;
                            break;
                        case InputKeyRight:
                            tanks_state->moving = true;
                            tanks_state->direction = DirectionRight;
                            break;
                        case InputKeyLeft:
                            tanks_state->moving = true;
                            tanks_state->direction = DirectionLeft;
                            break;
                        case InputKeyOk:
                            if(tanks_state->state == GameStateGameOver) {
                                tanks_game_init_game(tanks_state);
                            }
                            break;
                        case InputKeyBack:
                            processing = false;
                            break;
                    }
                }
            } else if(event.type == EventTypeTick) {
                tanks_game_process_game_step(tanks_state);
            }
        } else {
            // event timeout
        }

        view_port_update(view_port);
        release_mutex(&state_mutex, tanks_state);
    }

    osTimerDelete(timer);
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close("gui");
    view_port_free(view_port);
    osMessageQueueDelete(event_queue);
    delete_mutex(&state_mutex);
    free(tanks_state);

    return 0;
}
