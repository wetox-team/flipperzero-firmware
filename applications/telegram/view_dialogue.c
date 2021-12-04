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

static void view_telegram_draw_callback(Canvas* canvas, void* _model) {
    ViewTelegramModel* model = _model;
    view_dialogues[model->test](canvas, _model);
}

static void view_telegram_exit(void* context) {
    ViewTelegram* instance = context;
    osTimerStop(instance->timer);
}

ViewTelegram* view_telegram_alloc() {
    ViewTelegram* instance = furi_alloc(sizeof(ViewTelegram));

    instance->view = view_alloc();
    view_set_context(instance->view, instance);
    view_allocate_model(instance->view, ViewModelTypeLockFree, sizeof(ViewTelegramModel));
    view_set_draw_callback(instance->view, view_telegram_draw_callback);
    view_set_exit_callback(instance->view, view_telegram_exit);

    return instance;
}

View* view_telegram_get_view(ViewTelegram* instance) {
    furi_assert(instance);
    return instance->view;
}