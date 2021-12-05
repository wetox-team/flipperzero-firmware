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
#include <gui/modules/text_input.h>
#include <gui/modules/submenu.h>
#include <gui/modules/dialog_ex.h>
#include <gui/modules/popup.h>
#include <gui/modules/text_input.h>
#include <gui/modules/byte_input.h>
#include <gui/modules/text_box.h>
#include <gui/modules/widget.h>

#include <rpc/rpc_telegram.h>

#include "view_dialogue.h"

#define TAG "Telegram"
#define TG_TEXT_STORE_SIZE 128
#define TG_MESSAGE_MAX_LEN 20

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    ViewTelegram* view_dialogue;
    VariableItemList* variable_item_list;
    Submenu* submenu;
    TelegramApi* api;
    TextInput* text_input;

    char text_store[TG_TEXT_STORE_SIZE + 1];
} Telegram;

typedef enum {
    TelegramViewChats,
    TelegramViewDialogue,
    TelegramMessage,
    TelegramSendMessage,
    TelegramTextInput,
} TelegramView;

void telegram_send_message(void* context) {
    Telegram* instance = (Telegram*)context;
    view_dispatcher_switch_to_view(instance->view_dispatcher, TelegramViewChats);
}

void compose_message(void* context, uint32_t index) {
    Telegram* instance = (Telegram*)context;
    TextInput* text_input = instance->text_input;
    text_input_set_header_text(text_input, "Your message");
    text_input_set_result_callback(
        text_input,
        telegram_send_message,
        instance,
        instance->text_store,
        TG_MESSAGE_MAX_LEN,
        true);

    view_dispatcher_switch_to_view(instance->view_dispatcher, TelegramTextInput);
}

static uint32_t telegram_exit_callback(void* context) {
    return VIEW_NONE;
}

static void open_chat_callback(void* context, uint32_t index) {
    Telegram* instance = (Telegram*)context;
    submenu_clean(instance->submenu);
    submenu_add_item(instance->submenu, "Message 1", TelegramMessage, NULL, instance);
    submenu_add_item(instance->submenu, "Message 2", TelegramMessage, NULL, instance);
    submenu_add_item(instance->submenu, "Message 3", TelegramMessage, NULL, instance);
    submenu_add_item(instance->submenu, "Send...", TelegramSendMessage, compose_message, instance);

    return;
}

Telegram* telegram_init_chats_callback(Telegram* instance) {
    submenu_clean(instance->submenu);
    submenu_add_item(
        instance->submenu,
        "Chat 1 loaded",
        TelegramViewDialogue,
        open_chat_callback,
        instance);
    submenu_add_item(
        instance->submenu,
        "Chat 2 loaded",
        TelegramViewDialogue,
        open_chat_callback,
        instance);
    submenu_add_item(
        instance->submenu,
        "Chat 3 loaded",
        TelegramViewDialogue,
        open_chat_callback,
        instance);

    return instance;
}

Telegram* telegram_alloc() {
    Telegram* instance = furi_alloc(sizeof(Telegram));
    instance->text_input = text_input_alloc();

    View* view = NULL;

    // TelegramApi* api = furi_record_open("tg");
    // instance->api = api;
    instance->gui = furi_record_open("gui");
    instance->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_enable_queue(instance->view_dispatcher);
    view_dispatcher_attach_to_gui(
        instance->view_dispatcher, instance->gui, ViewDispatcherTypeFullscreen);

    instance->submenu = submenu_alloc();
    view = submenu_get_view(instance->submenu);
    view_set_previous_callback(view, telegram_exit_callback);
    view_dispatcher_add_view(
        instance->view_dispatcher, TelegramTextInput, text_input_get_view(instance->text_input));
    view_dispatcher_add_view(instance->view_dispatcher, TelegramViewChats, view);
    submenu_add_item(
        instance->submenu,
        "Chat 1",
        TelegramViewDialogue,
        open_chat_callback,
        instance);
    submenu_add_item(
        instance->submenu,
        "Chat 2",
        TelegramViewDialogue,
        open_chat_callback,
        instance);
    submenu_add_item(
        instance->submenu,
        "Chat 3",
        TelegramViewDialogue,
        open_chat_callback,
        instance);

    telegram_init_chats_callback(instance);
    return instance;
}

void telegram_free(Telegram* instance) {
    view_dispatcher_remove_view(instance->view_dispatcher, TelegramTextInput);
    text_input_free(instance->text_input);

    view_dispatcher_remove_view(instance->view_dispatcher, TelegramViewChats);
    submenu_free(instance->submenu);

    // view_dispatcher_free(instance->view_dispatcher);
    furi_record_close("gui");
    //furi_record_close("tg");

    free(instance);
}

int32_t telegram_run(Telegram* instance) {
    view_dispatcher_switch_to_view(instance->view_dispatcher, TelegramViewChats);
    view_dispatcher_run(instance->view_dispatcher);

    return 0;
}

int32_t telegram_app(void* p) {
    Telegram* instance = telegram_alloc();

    int32_t ret = telegram_run(instance);

    telegram_free(instance);

    return ret;
}