#pragma once

#include "magspoof.h"
#include "magspoof_device.h"

#include <furi.h>

#include <gui/gui.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <cli/cli.h>
#include <notification/notification-messages.h>

#include <gui/modules/submenu.h>
#include <gui/modules/dialog_ex.h>
#include <gui/modules/popup.h>
#include <gui/modules/text_input.h>
#include <gui/modules/byte_input.h>
#include <gui/modules/text_box.h>
#include <gui/modules/widget.h>

#include <magspoof/scenes/magspoof_scene.h>

#define MAGSPOOF_SEND_NOTIFICATION_FALSE (0UL)
#define MAGSPOOF_SEND_NOTIFICATION_TRUE (1UL)
#define MAGSPOOF_TEXT_STORE_SIZE 128
struct Magspoof {
    ViewDispatcher* view_dispatcher;
    Gui* gui;
    NotificationApp* notification;
    SceneManager* scene_manager;
    
    MagspoofDevice* dev;
    MagspoofDeviceCommonData dev_edit_data;

    char text_store[MAGSPOOF_TEXT_STORE_SIZE + 1];
    string_t text_box_store;

    View* view;
    FuriThread* worker_thread;
    StreamBufferHandle_t rx_stream;

    // Common Views
    Submenu* submenu;
    DialogEx* dialog_ex;
    Popup* popup;
    TextInput* text_input;
    ByteInput* byte_input;
    TextBox* text_box;
    Widget* widget;
};

typedef enum {
    MagspoofViewMenu,
    MagspoofViewDialogEx,
    MagspoofViewPopup,
    MagspoofViewTextInput,
    MagspoofViewByteInput,
    MagspoofViewTextBox,
    MagspoofViewWidget,
    MagspoofViewBankCard,
} MagspoofView;

// static Magspoof* magspoof_alloc();

int32_t magspoof_task(void* p);

void magspoof_text_store_set(Magspoof* magspoof, const char* text, ...);

void magspoof_text_store_clear(Magspoof* app);
