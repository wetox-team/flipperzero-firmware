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

typedef struct {
    Point coordinates;
    uint16_t score;
    Direction direction;
    GameState state;
    bool moving;
    // char map[FIELD_WIDTH][FIELD_HEIGHT];
    char map[16][11];
} SnakeState;

typedef enum {
    EventTypeTick,
    EventTypeKey,
} EventType;

typedef struct {
    EventType type;
    InputEvent input;
} SnakeEvent;

static void tanks_game_render_callback(Canvas* const canvas, void* ctx) {
    const SnakeState* snake_state = acquire_mutex((ValueMutex*)ctx, 25);
    if(snake_state == NULL) {
        return;
    }

    // Before the function is called, the state is set with the canvas_reset(canvas)

    canvas_draw_box(canvas, FIELD_WIDTH * CELL_LENGTH_PIXELS, 0, 2, SCREEN_HEIGHT);

    // Snake
    Point coordinates = snake_state->coordinates;

    switch (snake_state->direction) {
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
            switch (snake_state->map[x][y]) {
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
    if(snake_state->state == GameStateGameOver) {
        // Screen is 128x64 px
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, 34, 20, 62, 24);

        canvas_set_color(canvas, ColorBlack);
        canvas_draw_frame(canvas, 34, 20, 62, 24);

        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 37, 31, "Game Over");

        canvas_set_font(canvas, FontSecondary);
        char buffer[13];
        snprintf(buffer, sizeof(buffer), "Score: %u", snake_state->score);
        canvas_draw_str_aligned(canvas, 64, 41, AlignCenter, AlignBottom, buffer);
    }

    release_mutex((ValueMutex*)ctx, snake_state);
}

static void tanks_game_input_callback(InputEvent* input_event, osMessageQueueId_t event_queue) {
    furi_assert(event_queue);

    SnakeEvent event = {.type = EventTypeKey, .input = *input_event};
    osMessageQueuePut(event_queue, &event, 0, osWaitForever);
}

static void tanks_game_update_timer_callback(osMessageQueueId_t event_queue) {
    furi_assert(event_queue);

    SnakeEvent event = {.type = EventTypeTick};
    osMessageQueuePut(event_queue, &event, 0, 0);
}

static void tanks_game_init_game(SnakeState* const snake_state) {
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
            snake_state->map[x][y] = ' ';

            if (map[y][x] == '1') {
                c.x = x;
                c.y = y;
            }

            if (map[y][x] == '-') {
                snake_state->map[x][y] = '-';
            }

            if (map[y][x] == '=') {
                snake_state->map[x][y] = '=';
            }

            if (map[y][x] == '*') {
                snake_state->map[x][y] = '*';
            }

            if (map[y][x] == 'a') {
                snake_state->map[x][y] = 'a';
            }
        }
    }

    snake_state->coordinates = c;

    snake_state->score = 0;

    snake_state->moving = false;

    snake_state->direction = DirectionUp;

    snake_state->state = GameStateLife;
}

static bool tanks_game_collision_with_frame(Point const next_step) {
    // if x == 0 && currentMovement == left then x - 1 == 255 ,
    // so check only x > right border
    return next_step.x >= FIELD_WIDTH || next_step.y >= FIELD_HEIGHT;
}

static Point tanks_game_get_next_step(SnakeState const* const snake_state) {
    Point next_step = snake_state->coordinates;
    switch(snake_state->direction) {
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

static void tanks_game_move_snake(SnakeState* const snake_state, Point const next_step) {
    snake_state->coordinates = next_step;
}

static void tanks_game_process_game_step(SnakeState* const snake_state) {
    if(snake_state->state == GameStateGameOver) {
        return;
    }

    if(snake_state->moving) {
        Point next_step = tanks_game_get_next_step(snake_state);
        bool crush = tanks_game_collision_with_frame(next_step);

        if(!crush) {
            tanks_game_move_snake(snake_state, next_step);
        }
    }

    snake_state->moving = false;
}

int32_t tanks_game_app(void* p) {
    srand(DWT->CYCCNT);

    osMessageQueueId_t event_queue = osMessageQueueNew(8, sizeof(SnakeEvent), NULL);

    SnakeState* snake_state = furi_alloc(sizeof(SnakeState));
    tanks_game_init_game(snake_state);

    ValueMutex state_mutex;
    if(!init_mutex(&state_mutex, snake_state, sizeof(SnakeState))) {
        furi_log_print(FURI_LOG_ERROR, "cannot create mutex\r\n");
        free(snake_state);
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

    SnakeEvent event;
    for(bool processing = true; processing;) {
        osStatus_t event_status = osMessageQueueGet(event_queue, &event, NULL, 100);

        SnakeState* snake_state = (SnakeState*)acquire_mutex_block(&state_mutex);

        if(event_status == osOK) {
            // press events
            if(event.type == EventTypeKey) {
                if(event.input.type == InputTypePress) {
                    switch(event.input.key) {
                        case InputKeyUp:
                            snake_state->moving = true;
                            snake_state->direction = DirectionUp;
                            break;
                        case InputKeyDown:
                            snake_state->moving = true;
                            snake_state->direction = DirectionDown;
                            break;
                        case InputKeyRight:
                            snake_state->moving = true;
                            snake_state->direction = DirectionRight;
                            break;
                        case InputKeyLeft:
                            snake_state->moving = true;
                            snake_state->direction = DirectionLeft;
                            break;
                        case InputKeyOk:
                            if(snake_state->state == GameStateGameOver) {
                                tanks_game_init_game(snake_state);
                            }
                            break;
                        case InputKeyBack:
                            processing = false;
                            break;
                    }
                }
            } else if(event.type == EventTypeTick) {
                tanks_game_process_game_step(snake_state);
            }
        } else {
            // event timeout
        }

        view_port_update(view_port);
        release_mutex(&state_mutex, snake_state);
    }

    osTimerDelete(timer);
    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    furi_record_close("gui");
    view_port_free(view_port);
    osMessageQueueDelete(event_queue);
    delete_mutex(&state_mutex);
    free(snake_state);

    return 0;
}
