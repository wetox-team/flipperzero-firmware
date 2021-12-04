#include "../bt_settings_app.h"
#include "furi-hal-bt.h"
#include "furi-hal-ohs.h"

enum BtSetting {
    BtSettingOff,
    BtSettingOn,
    BtSettingOpenHaystack,
    BtSettingNum,
};

const char* const bt_settings_text[BtSettingNum] = {
    "Off",
    "On",
    "OHS"
};

static void bt_settings_scene_start_var_list_change_callback(VariableItem* item) {
    BtSettingsApp* app = variable_item_get_context(item);
    uint8_t index = variable_item_get_current_value_index(item);

    variable_item_set_current_value_text(item, bt_settings_text[index]);
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void bt_settings_scene_start_on_enter(void* context) {
    BtSettingsApp* app = context;
    VariableItemList* var_item_list = app->var_item_list;

    VariableItem* item;
    item = variable_item_list_add(
        var_item_list,
        "Bluetooth",
        BtSettingNum,
        bt_settings_scene_start_var_list_change_callback,
        app);
    if(app->settings.mode == BT_MODE_ON) {
        printf("Scene: Bluetooth: On\r\n");
        variable_item_set_current_value_index(item, BtSettingOn);
        variable_item_set_current_value_text(item, bt_settings_text[BtSettingOn]);
    } else if (app->settings.mode == BT_MODE_OFF) {
        printf("Scene: Bluetooth: Off\r\n");
        variable_item_set_current_value_index(item, BtSettingOff);
        variable_item_set_current_value_text(item, bt_settings_text[BtSettingOff]);
    } else if (app->settings.mode == BT_MODE_OHS) {
        printf("Scene: Bluetooth: OHS\r\n");
        variable_item_set_current_value_index(item, BtSettingOpenHaystack);
        variable_item_set_current_value_text(item, bt_settings_text[BtSettingOpenHaystack]);
    } else {
        printf("No possible scene!\r\n");
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, BtSettingsAppViewVarItemList);
}

bool bt_settings_scene_start_on_event(void* context, SceneManagerEvent event) {
    BtSettingsApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == BtSettingOn) {
            printf("Bluetooth: On\r\n");
            furi_hal_ohs_stop();
            furi_hal_bt_start_advertising();
            app->settings.mode = BT_MODE_ON;
        } else if(event.event == BtSettingOff) {
            printf("Bluetooth: Off\r\n");
            furi_hal_ohs_stop();
            furi_hal_bt_stop_advertising();
            app->settings.mode = BT_MODE_OFF;
        } else if(event.event == BtSettingOpenHaystack) {
            furi_hal_ohs_stop();
            furi_hal_bt_stop_advertising();
            printf("Bluetooth: OHS\r\n");
            app->settings.mode = BT_MODE_OHS;
            furi_hal_ohs_start();
        }
        consumed = true;
    }
    return consumed;
}

void bt_settings_scene_start_on_exit(void* context) {
    BtSettingsApp* app = context;
    variable_item_list_clean(app->var_item_list);
}
