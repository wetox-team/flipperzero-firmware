#include "../magspoof_i.h"

enum SubmenuIndex {
    SubmenuIndexEmulate,
    SubmenuIndexEdit,
    SubmenuIndexDelete,
    SubmenuIndexInfo,
};

void magspoof_scene_saved_menu_submenu_callback(void* context, uint32_t index) {
    Magspoof* app = (Magspoof*)context;

    view_dispatcher_send_custom_event(app->view_dispatcher, index);
}

void magspoof_scene_saved_menu_on_enter(void* context) {
    Magspoof* app = (Magspoof*)context;
    Submenu* submenu = app->submenu;

    submenu_add_item(
        submenu, "Emulate", SubmenuIndexEmulate, magspoof_scene_saved_menu_submenu_callback, app);
    submenu_add_item(
        submenu, "Edit", SubmenuIndexEdit, magspoof_scene_saved_menu_submenu_callback, app);
    submenu_add_item(
        submenu, "Delete", SubmenuIndexDelete, magspoof_scene_saved_menu_submenu_callback, app);
    submenu_add_item(
        submenu, "Info", SubmenuIndexInfo, magspoof_scene_saved_menu_submenu_callback, app);
    submenu_set_selected_item(
        app->submenu, scene_manager_get_scene_state(app->scene_manager, MagspoofSceneSavedMenu));
    

    view_dispatcher_switch_to_view(app->view_dispatcher, MagspoofViewMenu);
}

bool magspoof_scene_saved_menu_on_event(void* context, SceneManagerEvent event) {
    Magspoof* app = (Magspoof*)context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        scene_manager_set_scene_state(app->scene_manager, MagspoofSceneSavedMenu, event.event);
        if(event.event == SubmenuIndexEmulate) {
            // if(nfc->dev->format == NfcDeviceSaveFormatMifareUl) {
            //     scene_manager_next_scene(nfc->scene_manager, NfcSceneEmulateMifareUl);
            // } else {
            //     scene_manager_next_scene(nfc->scene_manager, NfcSceneEmulateUid);
            // }
            // consumed = true;
        } else if(event.event == SubmenuIndexEdit) {
            // scene_manager_next_scene(nfc->scene_manager, NfcSceneSetUid);
            // consumed = true;
        } else if(event.event == SubmenuIndexDelete) {
            // scene_manager_next_scene(app->scene_manager, MagspoofSceneDelete);
            // consumed = true;
        } else if(event.event == SubmenuIndexInfo) {
            // scene_manager_next_scene(nfc->scene_manager, NfcSceneDeviceInfo);
            // consumed = true;
        }
    }

    return consumed;
}

void magspoof_scene_saved_menu_on_exit(void* context) {
    Magspoof* app = (Magspoof*)context;

    submenu_clean(app->submenu);
}
