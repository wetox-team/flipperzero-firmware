#include "../magspoof_i.h"

#define MAGSPOOF_SCENE_READ_SUCCESS_SHIFT "              "

void magspoof_scene_read_card_success_dialog_callback(DialogExResult result, void* context) {
    Magspoof* magspoof = (Magspoof*)context;

    view_dispatcher_send_custom_event(magspoof->view_dispatcher, result);
}

void magspoof_scene_read_card_success_on_enter(void* context) {
    Magspoof* magspoof = (Magspoof*)context;

    notification_message(magspoof->notifications, &sequence_success);

    // Setup view
    MagspoofDeviceCommonData* data = &magspoof->dev->dev_data.magspoof_data;
    DialogEx* dialog_ex = magspoof->dialog_ex;
    dialog_ex_set_left_button_text(dialog_ex, "Emulate");
    dialog_ex_set_right_button_text(dialog_ex, "Save");
    dialog_ex_set_center_button_text(dialog_ex, "Clear");

    // XXX TODO: fill data

    dialog_ex_set_text(dialog_ex, nfc->text_store, 0, 0, AlignLeft, AlignTop);
    dialog_ex_set_context(dialog_ex, magspoof);
    dialog_ex_set_result_callback(dialog_ex, magspoof_scene_read_card_success_dialog_callback);

    view_dispatcher_switch_to_view(Magspoof->view_dispatcher, MagspoofViewDialogEx);
}

bool nfc_scene_read_card_success_on_event(void* context, SceneManagerEvent event) {
    Nfc* nfc = (Nfc*)context;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == DialogExResultLeft) {
            return scene_manager_previous_scene(nfc->scene_manager);
        } else if(event.event == DialogExResultRight) {
            // Clear device name
            nfc_device_set_name(nfc->dev, "");
            scene_manager_next_scene(nfc->scene_manager, NfcSceneCardMenu);
            return true;
        }
    }
    return false;
}

void nfc_scene_read_card_success_on_exit(void* context) {
    Nfc* nfc = (Nfc*)context;

    DialogEx* dialog_ex = nfc->dialog_ex;
    dialog_ex_set_header(dialog_ex, NULL, 0, 0, AlignCenter, AlignCenter);
    dialog_ex_set_text(dialog_ex, NULL, 0, 0, AlignCenter, AlignTop);
    dialog_ex_set_icon(dialog_ex, 0, 0, NULL);
    dialog_ex_set_left_button_text(dialog_ex, NULL);
    dialog_ex_set_right_button_text(dialog_ex, NULL);
    dialog_ex_set_result_callback(dialog_ex, NULL);
    dialog_ex_set_context(dialog_ex, NULL);
}
