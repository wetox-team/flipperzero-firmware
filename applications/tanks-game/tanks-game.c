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
    bool explosion;
    bool is_p1;
    bool is_p2;
} ProjectileState;

typedef struct {
    Point coordinates;
    uint16_t score;
    uint8_t lives;
    Direction direction;
    bool moving;
    bool shooting;
    bool live;
    uint8_t cooldown;
    uint8_t respawn_cooldown;
} PlayerState;

typedef struct {
    // char map[FIELD_WIDTH][FIELD_HEIGHT];
    char map[16][11];
    Point team_one_respawn_points[3];
    Point team_two_respawn_points[3];
    Mode mode;
    GameState state;
    ProjectileState *projectiles[100];
    PlayerState *bots[6];
    uint8_t enemies_left;
    uint8_t enemies_live;
    uint8_t enemies_respawn_cooldown;
    PlayerState *p1;
    PlayerState *p2;
} TanksState;

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
    Point coordinates = tanks_state->p1->coordinates;

    switch (tanks_state->p1->direction) {
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

    for (
        uint8_t i = 0;
        i < 6;
        i++
    ) {
        if (tanks_state->bots[i] != NULL) {
            canvas_draw_icon(
                canvas,
                tanks_state->bots[i]->coordinates.x * CELL_LENGTH_PIXELS,
                tanks_state->bots[i]->coordinates.y * CELL_LENGTH_PIXELS - 1,
                &I_enemy_left);
        }
    }

    for(int8_t x = 0; x < 100; x++) {
        if (tanks_state->projectiles[x] != NULL) {
            ProjectileState *projectile = tanks_state->projectiles[x];

            if (projectile->explosion) {
                canvas_draw_icon(
                    canvas,
                    projectile->coordinates.x * CELL_LENGTH_PIXELS,
                    projectile->coordinates.y * CELL_LENGTH_PIXELS - 1,
                    &I_tank_explosion);
                continue;
            }

            switch(projectile->direction) {
            case DirectionUp:
                canvas_draw_icon(
                    canvas,
                    projectile->coordinates.x * CELL_LENGTH_PIXELS,
                    projectile->coordinates.y * CELL_LENGTH_PIXELS - 1,
                    &I_projectile_up);
                break;
            case DirectionDown:
                canvas_draw_icon(
                    canvas,
                    projectile->coordinates.x * CELL_LENGTH_PIXELS,
                    projectile->coordinates.y * CELL_LENGTH_PIXELS - 1,
                    &I_projectile_down);
                break;
            case DirectionRight:
                canvas_draw_icon(
                    canvas,
                    projectile->coordinates.x * CELL_LENGTH_PIXELS,
                    projectile->coordinates.y * CELL_LENGTH_PIXELS - 1,
                    &I_projectile_right);
                break;
            case DirectionLeft:
                canvas_draw_icon(
                    canvas,
                    projectile->coordinates.x * CELL_LENGTH_PIXELS,
                    projectile->coordinates.y * CELL_LENGTH_PIXELS - 1,
                    &I_projectile_left);
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

        if (tanks_state->enemies_left == 0 && tanks_state->enemies_live == 0) {
            canvas_draw_str(canvas, 42, 31, "You win!");
        } else {
            canvas_draw_str(canvas, 37, 31, "Game Over");
        }

        canvas_set_font(canvas, FontSecondary);
        char buffer[13];
        snprintf(buffer, sizeof(buffer), "Score: %u", tanks_state->p1->score);
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
    srand(DWT->CYCCNT);

    //char map[FIELD_HEIGHT][FIELD_WIDTH + 1] = {
    char map[11][16 + 1] = {
        "*       -  *   -",
        "  -  -  =       ",
        "        -  -   2",
        "1    =     - -- ",
        "--   =     - -- ",
        "a-1  =  -  =   2",
        "--   =     - -- ",
        "1    =     - -- ",
        "        -  -   2",
        "  -  -  =       ",
        "*       -  *   -",
    };

    for(int8_t x = 0; x < 100; x++) {
        if(tanks_state->projectiles[x] != NULL) {
            free(tanks_state->projectiles[x]);
            tanks_state->projectiles[x] = NULL;
        }
    }

    int8_t team_one_respawn_points_counter = 0;
    int8_t team_two_respawn_points_counter = 0;

    for(int8_t x = 0; x < FIELD_WIDTH; x++) {
        for(int8_t y = 0; y < FIELD_HEIGHT; y++) {
            tanks_state->map[x][y] = ' ';

            if (map[y][x] == '1') {
                Point respawn = {x, y};
                tanks_state->team_one_respawn_points[team_one_respawn_points_counter++] = respawn;
            }

            if (map[y][x] == '2') {
                Point respawn = {x, y};
                tanks_state->team_two_respawn_points[team_two_respawn_points_counter++] = respawn;
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

    uint8_t respawn_point_index = rand() % 3;
    Point c = {tanks_state->team_one_respawn_points[respawn_point_index].x, tanks_state->team_one_respawn_points[respawn_point_index].y};

    PlayerState p1 = {
        c,
        0,
        4,
        DirectionRight,
        0,
        0,
        1,
        SHOT_COOLDOWN,
        PLAYER_RESPAWN_COOLDOWN,
    };

    PlayerState* p1_state = furi_alloc(sizeof(PlayerState));
    *p1_state = p1;

    tanks_state->p1 = p1_state;
    tanks_state->state = GameStateLife;
    tanks_state->enemies_left = 5;
    tanks_state->enemies_live = 0;
    tanks_state->enemies_respawn_cooldown = RESPAWN_COOLDOWN;
}

static bool tanks_game_collision(Point const next_step, bool shoot, TanksState const* const tanks_state) {
    if (next_step.x < 0 || next_step.y < 0) {
        return true;
    }

    if (next_step.x >= FIELD_WIDTH || next_step.y >= FIELD_HEIGHT) {
        return true;
    }

    char tile = tanks_state->map[next_step.x][next_step.y];

    if (tile == '*' && !shoot) {
        return true;
    }

    if (tile == '-' || tile == '=' || tile == 'a') {
        return true;
    }

    for (
        uint8_t i = 0;
        i < 6;
        i++
    ) {
        if (tanks_state->bots[i] != NULL) {
            if (
                tanks_state->bots[i]->coordinates.x == next_step.x &&
                tanks_state->bots[i]->coordinates.y == next_step.y
                ) {
                return true;
            }
        }
    }

    return false;
}

static Point tanks_game_get_next_step(Point coordinates, Direction direction) {
    Point next_step = {
        coordinates.x,
        coordinates.y
    };

    switch(direction) {
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

static bool tanks_get_cell_is_free(TanksState* const tanks_state, Point point) {
    // Tiles
    if (tanks_state->map[point.x][point.y] != ' ') {
        return false;
    }

    // Projectiles
    for(int8_t x = 0; x < 100; x++) {
        if(tanks_state->projectiles[x] != NULL) {
            if (
                tanks_state->projectiles[x]->coordinates.x == point.x &&
                tanks_state->projectiles[x]->coordinates.y == point.y
                ) {
                return false;
            }
        }
    }

    // Player 1
    if (tanks_state->p1 != NULL) {
        if (
            tanks_state->p1->coordinates.x == point.x &&
            tanks_state->p1->coordinates.y == point.y
           ) {
            return false;
        }
    }

    // Player 2
    if (tanks_state->p2 != NULL) {
        if (
            tanks_state->p2->coordinates.x == point.x &&
            tanks_state->p2->coordinates.y == point.y
        ) {
            return false;
        }
    }

    // Bots
    for(int8_t x = 0; x < 6; x++) {
        if(tanks_state->bots[x] != NULL) {
            if (
                tanks_state->bots[x]->coordinates.x == point.x &&
                tanks_state->bots[x]->coordinates.y == point.y
            ) {
                return false;
            }
        }
    }

    return true;
}

static uint8_t tanks_get_random_free_respawn_point_index(
    TanksState* const tanks_state,
    Point respawn_points[3]
    ) {

    uint8_t first = rand() % 3;
    int8_t add = rand() % 2 ? +1 : -1;
    int8_t second = first + add;
    uint8_t third;

    if (second == 4) {
        second = 0;
    } else if (second == -1) {
        second = 3;
    }

    for (uint8_t i = 0; i < 3; i++) {
        if (i != first && i != second) {
            third = i;
        }
    }

    if (tanks_get_cell_is_free(tanks_state, respawn_points[first])) {
        return first;
    }

    if (tanks_get_cell_is_free(tanks_state, respawn_points[second])) {
        return second;
    }

    if (tanks_get_cell_is_free(tanks_state, respawn_points[third])) {
        return third;
    }

    return -1;
}

static void tanks_game_process_game_step(TanksState* const tanks_state) {
    if(tanks_state->enemies_left == 0 && tanks_state->enemies_live == 0) {
        tanks_state->state = GameStateGameOver;
    }

    if(tanks_state->state == GameStateGameOver) {
        return;
    }

    if (tanks_state->enemies_respawn_cooldown) {
        tanks_state->enemies_respawn_cooldown--;
    }

    if (
        tanks_state->enemies_left > 0 &&
        tanks_state->enemies_live <= 4 &&
        tanks_state->enemies_respawn_cooldown == 0)
    {
        int8_t index = tanks_get_random_free_respawn_point_index(
            tanks_state,
            tanks_state->team_two_respawn_points
            );

        if (index != -1) {
            tanks_state->enemies_left--;
            tanks_state->enemies_live++;
            tanks_state->enemies_respawn_cooldown = RESPAWN_COOLDOWN;
            Point point = tanks_state->team_two_respawn_points[index];

            Point c = {point.x, point.y};

            PlayerState bot = {
                c,
                0,
                0,
                DirectionLeft,
                0,
                0,
                1,
                SHOT_COOLDOWN,
                PLAYER_RESPAWN_COOLDOWN,
            };

            uint8_t freeEnemyIndex;
            for (
                freeEnemyIndex = 0;
                freeEnemyIndex < 6;
                freeEnemyIndex++
            ) {
                if (tanks_state->bots[freeEnemyIndex] == NULL) {
                    break;
                }
            }

            PlayerState* bot_state = furi_alloc(sizeof(PlayerState));
            *bot_state = bot;

            tanks_state->bots[freeEnemyIndex] = bot_state;
        }
    }

    if(tanks_state->p1->moving) {
        Point next_step = tanks_game_get_next_step(tanks_state->p1->coordinates, tanks_state->p1->direction);
        bool crush = tanks_game_collision(next_step, false, tanks_state);

        if(!crush) {
            tanks_state->p1->coordinates = next_step;
        }
    }

    for(int8_t x = 0; x < 100; x++) {
        if(tanks_state->projectiles[x] != NULL) {
            ProjectileState *projectile = tanks_state->projectiles[x];
            Point c = projectile->coordinates;

            if (projectile->explosion) {
                // Break a wall
                if (tanks_state->map[c.x][c.y] == '-') {
                    tanks_state->map[c.x][c.y] = ' ';
                }

                // Kill a bot
                for (
                    uint8_t i = 0;
                    i < 6;
                    i++
                ) {
                    if (tanks_state->bots[i] != NULL) {
                        if (
                            tanks_state->bots[i]->coordinates.x == c.x &&
                            tanks_state->bots[i]->coordinates.y == c.y
                            ) {
                            tanks_state->enemies_live--;
                            free(tanks_state->bots[i]);
                            tanks_state->bots[i] = NULL;

                            if (projectile->is_p1) {
                                tanks_state->p1->score++;
                            }

                            if (projectile->is_p2) {
                                tanks_state->p2->score++;
                            }
                        }
                    }
                }

                // Destroy the flag
                if (tanks_state->map[c.x][c.y] == 'a') {
                    tanks_state->state = GameStateGameOver;
                    return;
                }

                free(tanks_state->projectiles[x]);
                tanks_state->projectiles[x] = NULL;
                continue;
            }

            Point next_step = tanks_game_get_next_step(projectile->coordinates, projectile->direction);
            bool crush = tanks_game_collision(next_step, true, tanks_state);
            projectile->coordinates = next_step;

            if(crush) {
                projectile->explosion = true;
            }
        }
    }

    if (tanks_state->p1->cooldown > 0) {
        tanks_state->p1->cooldown--;
    }

    if(tanks_state->p1->shooting && tanks_state->p1->cooldown == 0) {
        tanks_state->p1->cooldown = SHOT_COOLDOWN;

        uint8_t freeProjectileIndex;
        for (
                freeProjectileIndex = 0;
                freeProjectileIndex < 100;
                freeProjectileIndex++
            ) {
            if (tanks_state->projectiles[freeProjectileIndex] == NULL) {
                break;
            }
        }

        ProjectileState* projectile_state = furi_alloc(sizeof(ProjectileState));
        Point next_step = tanks_game_get_next_step(tanks_state->p1->coordinates, tanks_state->p1->direction);

        projectile_state->direction = tanks_state->p1->direction;
        projectile_state->coordinates = next_step;
        projectile_state->is_p1 = true;
        projectile_state->is_p2 = false;

        bool crush = tanks_game_collision(projectile_state->coordinates, true, tanks_state);
        projectile_state->explosion = crush;

        tanks_state->projectiles[freeProjectileIndex] = projectile_state;
    }

    tanks_state->p1->moving = false;
    tanks_state->p1->shooting = false;
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
                            tanks_state->p1->moving = true;
                            tanks_state->p1->direction = DirectionUp;
                            break;
                        case InputKeyDown:
                            tanks_state->p1->moving = true;
                            tanks_state->p1->direction = DirectionDown;
                            break;
                        case InputKeyRight:
                            tanks_state->p1->moving = true;
                            tanks_state->p1->direction = DirectionRight;
                            break;
                        case InputKeyLeft:
                            tanks_state->p1->moving = true;
                            tanks_state->p1->direction = DirectionLeft;
                            break;
                        case InputKeyOk:
                            if(tanks_state->state == GameStateGameOver) {
                                tanks_game_init_game(tanks_state);
                            } else {
                                tanks_state->p1->shooting = true;
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

    if (tanks_state->p1 != NULL) {
        free(tanks_state->p1);
    }

    if (tanks_state->p2 != NULL) {
        free(tanks_state->p2);
    }

    free(tanks_state);

    return 0;
}
