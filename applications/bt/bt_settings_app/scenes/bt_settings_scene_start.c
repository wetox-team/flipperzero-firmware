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
    "OpenHaystack"
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
    if(app->settings.enabled && !app->settings.ohs_enabled) {
        variable_item_set_current_value_index(item, BtSettingOn);
        variable_item_set_current_value_text(item, bt_settings_text[BtSettingOn]);
    } else if (!app->settings.enabled && !app->settings.ohs_enabled) {
        variable_item_set_current_value_index(item, BtSettingOff);
        variable_item_set_current_value_text(item, bt_settings_text[BtSettingOff]);
    } else {
        variable_item_set_current_value_index(item, BtSettingOpenHaystack);
        variable_item_set_current_value_text(item, bt_settings_text[BtSettingOpenHaystack]);
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, BtSettingsAppViewVarItemList);
}

bool bt_settings_scene_start_on_event(void* context, SceneManagerEvent event) {
    BtSettingsApp* app = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == BtSettingOn) {
            furi_hal_ohs_stop();
            furi_hal_bt_start_advertising();
            app->settings.enabled = true;
            app->settings.ohs_enabled = false;
        } else if(event.event == BtSettingOff) {
            app->settings.enabled = false;
            app->settings.ohs_enabled = false;
            furi_hal_ohs_stop();
            furi_hal_bt_stop_advertising();
        } else if(event.event == BtSettingOpenHaystack) {
            app->settings.enabled = false;
            app->settings.ohs_enabled = true;
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
