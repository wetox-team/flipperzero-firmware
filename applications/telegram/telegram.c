#include "telegram.h"

#include <furi-hal.h>
#include <furi.h>

// Need access to u8g2
#include <gui/gui_i.h>
#include <gui/canvas_i.h>
#include <u8g2_glue.h>

#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/variable-item-list.h>

#include "view_dialogue.h"

#define TAG "Telegram"

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    ViewTelegram* view_dialogue;
    VariableItemList* variable_item_list;
    Submenu* submenu;
} Telegram;

typedef enum {
    TelegramViewDialogue,
} TelegramView;

static void telegram_submenu_callback(void* context, uint32_t index) {
    Telegram* instance = (Telegram*)context;
    view_dispatcher_switch_to_view(instance->view_dispatcher, index);
}

static uint32_t telegram_exit_callback(void* context) {
    return VIEW_NONE;
}

Telegram* telegram_alloc() {
    Telegram* instance = furi_alloc(sizeof(Telegram));

    View* view = NULL;

    instance->gui = furi_record_open("gui");
    instance->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(instance->view_dispatcher);
    view_dispatcher_attach_to_gui(
        instance->view_dispatcher, instance->gui, ViewDispatcherTypeFullscreen);
    // Menu
    instance->submenu = submenu_alloc();
    view = submenu_get_view(instance->submenu);
    view_set_previous_callback(view, telegram_exit_callback);
    view_dispatcher_add_view(instance->view_dispatcher, TelegramViewDialogue, view);
    submenu_add_item(
        instance->submenu,
        "Chat 1",
        TelegramViewDialogue,
        telegram_submenu_callback,
        instance);
    submenu_add_item(
        instance->submenu,
        "Chat 2",
        TelegramViewDialogue,
        telegram_submenu_callback,
        instance);
    submenu_add_item(
        instance->submenu,
        "Chat 3",
        TelegramViewDialogue,
        telegram_submenu_callback,
        instance);

    return instance;
}

void telegram_free(Telegram* instance) {
    view_dispatcher_remove_view(instance->view_dispatcher, TelegramViewDialogue);
    submenu_free(instance->submenu);

    view_dispatcher_remove_view(instance->view_dispatcher, TelegramViewDialogue);
    variable_item_list_free(instance->variable_item_list);

    view_dispatcher_free(instance->view_dispatcher);
    furi_record_close("gui");

    free(instance);
}

int32_t telegram_run(Telegram* instance) {
    view_dispatcher_switch_to_view(instance->view_dispatcher, TelegramViewDialogue);
    view_dispatcher_run(instance->view_dispatcher);

    return 0;
}

int32_t telegram_app(void* p) {
    Telegram* instance = telegram_alloc();

    int32_t ret = telegram_run(instance);

    telegram_free(instance);

    return ret;
}