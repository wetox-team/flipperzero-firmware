#pragma once

#include <stdint.h>
#include <gui/view.h>

typedef struct ViewTelegram ViewTelegram;

ViewTelegram* view_dialogue_alloc();

void view_telegram_free(ViewTelegram* instance);

View* view_dialogue_get_view(ViewTelegram* instance);
