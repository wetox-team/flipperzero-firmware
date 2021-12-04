#include "view_dialogue.h"

typedef struct {
    uint32_t test;
    uint32_t size;
    uint32_t counter;
    bool flip_flop;
} ViewTelegramModel;

struct ViewTelegram {
    View* view;
    osTimerId_t timer;
};

static void view_dialogue_draw_callback_intro(Canvas* canvas, void* _model) {
    canvas_draw_str(canvas, 12, 24, "cum");
}

ViewDrawCallback view_dialogues[] = {
    view_dialogue_draw_callback_intro,
};

void view_telegram_free(ViewTelegram* instance) {
    furi_assert(instance);

    osTimerDelete(instance->timer);
    view_free(instance->view);
    free(instance);
}