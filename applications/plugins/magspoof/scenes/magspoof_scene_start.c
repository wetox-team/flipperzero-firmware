#include "../magspoof_i.h"

enum SubmenuIndex {
    SubmenuIndexRead,
    SubmenuIndexRunScript,
    SubmenuIndexSaved,
};

void magspoof_scene_start_submenu_callback(void* context, uint32_t index) {
    Magspoof* magspoof = (Magspoof*)context;

    view_dispatcher_send_custom_event(magspoof->view_dispatcher, index);
}

void magspoof_scene_start_on_enter(void* context) {
    Magspoof* magspoof = (Magspoof*)context;
    Submenu* submenu = magspoof->submenu;

    submenu_add_item(
        submenu, "Read card", SubmenuIndexRead, magspoof_scene_start_submenu_callback, magspoof);
    submenu_add_item(
        submenu, "Saved cards", SubmenuIndexSaved, magspoof_scene_start_submenu_callback, magspoof);
    submenu_add_item(
        submenu,
        "Run special action",
        SubmenuIndexRunScript,
        magspoof_scene_start_submenu_callback,
        magspoof);
    submenu_set_selected_item(
        submenu, scene_manager_get_scene_state(magspoof->scene_manager, MagspoofSceneStart));


    view_dispatcher_switch_to_view(magspoof->view_dispatcher, MagspoofViewMenu);
}

bool magspoof_scene_start_on_event(void* context, SceneManagerEvent event) {
    Magspoof* magspoof = (Magspoof*)context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == SubmenuIndexRead) {
            scene_manager_set_scene_state(magspoof->scene_manager, MagspoofSceneStart, SubmenuIndexRead);
            scene_manager_next_scene(magspoof->scene_manager, MagspoofSceneReadCard);
            consumed = true;
        } else if(event.event == SubmenuIndexRunScript) {
            // scene_manager_set_scene_state(
            //     magspoof->scene_manager, MagspoofSceneStart, SubmenuIndexRunScript);
            // scene_manager_next_scene(magspoof->scene_manager, MagspoofSceneScriptsMenu);
            consumed = true;
        } else if(event.event == SubmenuIndexSaved) {
            scene_manager_set_scene_state(magspoof->scene_manager, MagspoofSceneStart, SubmenuIndexSaved);
            scene_manager_next_scene(magspoof->scene_manager, MagspoofSceneFileSelect);
            consumed = true;
        }
    }
    return consumed;
}

void magspoof_scene_start_on_exit(void* context) {
    Magspoof* magspoof = (Magspoof*)context;

    submenu_free(magspoof->submenu);
}
