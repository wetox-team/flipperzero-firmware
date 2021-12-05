#include "../magspoof_i.h"
#include "../magspoof_device.h"

void magspoof_scene_file_select_on_enter(void* context) {
    Magspoof* app = (Magspoof*)context;
    if(magspoof_file_select(app->dev)) {
        scene_manager_next_scene(app->scene_manager, MagspoofSceneSavedMenu);
    } else {
        scene_manager_search_and_switch_to_previous_scene(app->scene_manager, MagspoofSceneStart);
    }
}

bool magspoof_scene_file_select_on_event(void* context, SceneManagerEvent event) {
    return false;
}

void magspoof_scene_file_select_on_exit(void* context) {
}
