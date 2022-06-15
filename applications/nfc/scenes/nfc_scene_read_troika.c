#include "../nfc_i.h"

enum {
    NfcSceneReadTroikaStateInProgress,
    NfcSceneReadTroikaStateDone,
};

void nfc_read_troika_worker_callback(NfcWorkerEvent event, void* context) {
    furi_assert(context);
    Nfc* nfc = context;
    view_dispatcher_send_custom_event(nfc->view_dispatcher, event);
}

void nfc_read_troika_dict_attack_result_callback(void* context) {
    furi_assert(context);
    Nfc* nfc = context;
    view_dispatcher_send_custom_event(nfc->view_dispatcher, NfcCustomEventDictAttackDone);
}

void nfc_scene_read_troika_on_enter(void* context) {
    Nfc* nfc = context;

    // Setup and start worker
    memset(&nfc->dev->dev_data.mf_classic_data, 0, sizeof(MfClassicData));
    dict_attack_set_result_callback(
        nfc->dict_attack, nfc_read_troika_dict_attack_result_callback, nfc);
    scene_manager_set_scene_state(
        nfc->scene_manager, NfcSceneReadTroika, NfcSceneReadTroikaStateInProgress);
    view_dispatcher_switch_to_view(nfc->view_dispatcher, NfcViewDictAttack);
    bool is_troika = true;
    memcpy(&nfc->dev->dev_data.is_troika, &is_troika, sizeof(bool));
    nfc_worker_start(
        nfc->worker,
        NfcWorkerStateReadMifareClassic,
        &nfc->dev->dev_data,
        nfc_read_troika_worker_callback,
        nfc);
}

bool nfc_scene_read_troika_on_event(void* context, SceneManagerEvent event) {
    Nfc* nfc = context;
    bool consumed = false;

    uint32_t state = scene_manager_get_scene_state(nfc->scene_manager, NfcSceneReadTroika);
    if(event.type == SceneManagerEventTypeTick) {
        if(state == NfcSceneReadTroikaStateInProgress) {
            notification_message(nfc->notifications, &sequence_blink_blue_10);
        }
        consumed = true;
    } else if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == NfcCustomEventDictAttackDone) {
            scene_manager_next_scene(nfc->scene_manager, NfcSceneMifareClassicMenu);
            consumed = true;
        } else if(event.event == NfcWorkerEventDetectedClassic1k) {
            dict_attack_card_detected(nfc->dict_attack, MfClassicType1k);
            consumed = true;
        } else if(event.event == NfcWorkerEventDetectedClassic4k) {
            dict_attack_card_detected(nfc->dict_attack, MfClassicType4k);
            consumed = true;
        } else if(event.event == NfcWorkerEventNewSector) {
            dict_attack_inc_curr_sector(nfc->dict_attack);
            consumed = true;
        } else if(event.event == NfcWorkerEventFoundKeyA) {
            dict_attack_inc_found_key(nfc->dict_attack, MfClassicKeyA);
            consumed = true;
        } else if(event.event == NfcWorkerEventFoundKeyB) {
            dict_attack_inc_found_key(nfc->dict_attack, MfClassicKeyB);
            consumed = true;
        } else if(event.event == NfcWorkerEventNoCardDetected) {
            dict_attack_card_removed(nfc->dict_attack);
            consumed = true;
        } else if(event.event == NfcWorkerEventSuccess) {
            scene_manager_set_scene_state(
                nfc->scene_manager, NfcSceneReadTroika, NfcSceneReadTroikaStateDone);
            notification_message(nfc->notifications, &sequence_success);
            dict_attack_set_result(nfc->dict_attack, true);
            consumed = true;
        } else if(event.event == NfcWorkerEventFail) {
            scene_manager_set_scene_state(
                nfc->scene_manager, NfcSceneReadTroika, NfcSceneReadTroikaStateDone);
            dict_attack_set_result(nfc->dict_attack, false);
            consumed = true;
        } else if(event.event == NfcWorkerEventNoDictFound) {
            scene_manager_next_scene(nfc->scene_manager, NfcSceneDictNotFound);
            consumed = true;
        }
    }
    return consumed;
}

void nfc_scene_read_troika_on_exit(void* context) {
    Nfc* nfc = context;
    // Stop worker
    nfc_worker_stop(nfc->worker);
    dict_attack_reset(nfc->dict_attack);
}
