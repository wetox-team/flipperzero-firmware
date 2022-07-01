#include "../desktop_settings_app.h"
#include "applications.h"
#include "desktop_settings_scene.h"


#define BATTERY_VIEW_COUNT 6


const char* const battery_view_count_text[BATTERY_VIEW_COUNT] = {
    "Bar",
    "%",
    "Inv. %",
    "Retro 3",
    "Retro 5",
    "Faces",
};
static void desktop_settings_scene_battery_submenu_callback(void* context, uint32_t index) {
    DesktopSettingsApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void desktop_settings_scene_battery_on_enter(void* context) {
    DesktopSettingsApp* app = context;

    Submenu* submenu = app->submenu;
    submenu_reset(submenu);

    for(size_t i = 0; i < BATTERY_VIEW_COUNT; i++) {
        submenu_add_item(
            submenu,
            battery_view_count_text[i],
            i,
            desktop_settings_scene_battery_submenu_callback,
            app);
    }


    submenu_set_header(app->submenu, "Battery View Type:");
    submenu_set_selected_item(app->submenu, app->settings.displayBatteryPercentage);


    view_dispatcher_switch_to_view(app->view_dispatcher, DesktopSettingsAppViewMenu);
}

bool desktop_settings_scene_battery_on_event(void* context, SceneManagerEvent event) {
    DesktopSettingsApp* app = context;

    bool consumed = false;
    if(event.type == SceneManagerEventTypeCustom) {
        app->settings.displayBatteryPercentage = event.event;
        scene_manager_previous_scene(app->scene_manager);
        consumed = true;
    }

    return consumed;
}


void desktop_settings_scene_battery_on_exit(void* context) {
    DesktopSettingsApp* app = context;
    SAVE_DESKTOP_SETTINGS(&app->settings);
    submenu_reset(app->submenu);
}
