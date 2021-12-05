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
#define TG_TEXT_STORE_SIZE 20
#define TG_MESSAGE_MAX_LEN TG_TEXT_STORE_SIZE

typedef struct {
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    ViewTelegram* view_dialogue;
    VariableItemList* variable_item_list;
    Submenu* submenu;
    TelegramApi* api;
    TextInput* text_input;
    PB_Telegram_TelegramStateResponse* response;
    int32_t currId;

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
    instance->api->send_msg(instance->api->rpc, instance->currId, instance->text_store);

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
    instance->currId = instance->response->dialogs[index-1].id;
    submenu_clean(instance->submenu);
    FURI_LOG_I(TAG, "%i dc", instance->response->dialogs_count);
    FURI_LOG_I(TAG, "%i mc", instance->response->dialogs[index-1].messages_count);
    for (int32_t i = 0; i < instance->response->dialogs[index-1].messages_count; i++)
    {
        submenu_add_item(instance->submenu, instance->response->dialogs[index-1].messages[i].text, TelegramMessage, NULL, instance);
    }
    
    submenu_add_item(instance->submenu, "Send...", TelegramSendMessage, compose_message, instance);

    return;
}

void telegram_init_chats_callback(const PB_Telegram_TelegramStateResponse* response, void* context) {
    Telegram* instance = (Telegram*)context;
    submenu_clean(instance->submenu);

    if (response == NULL) {
        submenu_add_item(
            instance->submenu,
            "Chats are loading...",
            TelegramViewDialogue,
            NULL,
            instance);
    } else {
        memcpy(instance->response, response, sizeof(PB_Telegram_TelegramStateResponse));
        for (int32_t i = 0; i < response->dialogs_count; i++)
        {
            FURI_LOG_I(TAG, "%s", instance->response->dialogs[i].name);
            instance->response->dialogs[i].name = strdup(response->dialogs[i].name);

            for (int32_t j = 0; j < response->dialogs[i].messages_count; j++)
            {
                instance->response->dialogs[i].messages[j].text = strdup(response->dialogs[i].messages[j].text);
                FURI_LOG_I(TAG, "%s", instance->response->dialogs[i].messages[j].text);
                FURI_LOG_I(TAG, "%s", response->dialogs[i].messages[j].text);
            }
            
            instance->response->dialogs[i].messages_count = response->dialogs[i].messages_count;
            submenu_add_item(
                instance->submenu,
                instance->response->dialogs[i].name,
                TelegramViewDialogue,
                open_chat_callback,
                instance);
        }
    }

    return;

}

Telegram* telegram_alloc() {
    Telegram* instance = furi_alloc(sizeof(Telegram));
    instance->text_input = text_input_alloc();

    View* view = NULL;

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

    if (!furi_record_exists("tg"))  {
        submenu_add_item(
            instance->submenu,
            "NO RPC",
            TelegramViewDialogue,
            NULL,
            instance);

        return instance;
    }

    telegram_init_chats_callback(NULL, instance);
    TelegramApi* api = furi_record_open("tg");
    instance->response = furi_alloc(sizeof(PB_Telegram_TelegramStateResponse));
    api->instance = instance;
    instance->api = api;
    api->handler = telegram_init_chats_callback;
    api->get_state(api->rpc);
    return instance;
}

void telegram_free(Telegram* instance) {
    view_dispatcher_remove_view(instance->view_dispatcher, TelegramTextInput);
    text_input_free(instance->text_input);

    view_dispatcher_remove_view(instance->view_dispatcher, TelegramViewChats);
    submenu_free(instance->submenu);

    // view_dispatcher_free(instance->view_dispatcher);
    furi_record_close("gui");

    if (furi_record_exists("tg"))
        furi_record_close("tg");

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
