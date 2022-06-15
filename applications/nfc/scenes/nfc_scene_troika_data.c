#include "../nfc_i.h"
#include <dolphin/dolphin.h>

void nfc_scene_troika_data_widget_callback(GuiButtonType result, InputType type, void* context) {
    Nfc* nfc = context;
    if(type == InputTypeShort) {
        view_dispatcher_send_custom_event(nfc->view_dispatcher, result);
    }
}

void nfc_scene_troika_data_on_enter(void* context) {
    Nfc* nfc = context;
    MfClassicData* troika_data = &nfc->dev->dev_data.mf_classic_data;
    FuriHalNfcDevData* nfc_data = &nfc->dev->dev_data.nfc_data;
    DOLPHIN_DEED(DolphinDeedNfcReadSuccess);

    // Setup Custom Widget view
    // Add frame
    widget_add_frame_element(nfc->widget, 0, 0, 128, 64, 6);
    // Add buttons
    widget_add_button_element(
        nfc->widget, GuiButtonTypeLeft, "Back", nfc_scene_troika_data_widget_callback, nfc);
    widget_add_button_element(
        nfc->widget, GuiButtonTypeRight, "Save", nfc_scene_troika_data_widget_callback, nfc);
    // Add card name
    widget_add_string_element(nfc->widget, 64, 3, AlignCenter, AlignTop, FontSecondary, "TROIKA");
    // Add card number

    // ----------------
    // CURSED CODE BEGINS HERE
    // ----------------

    // Get number from block 8 bytes 3-7
    uint8_t number[] = {0x00, 0x00, 0x00, 0x00};

    for(uint8_t i = 0; i < 4; i++) {
        number[i] = troika_data->block[8 * 4].value[i + 3];
    }

    // Remove the second half of the last byte
    number[3] &= 0xF0;

    // Convert number to string
    uint32_t number_int = 0;
    for(uint8_t i = 0; i < 4; i++) {
        number_int <<= 8;
        number_int |= number[i];
    }
    number_int >>= 4;

    // Convert number to string
    char* num_string;
    num_string = malloc(sizeof(char) * 16);
    sprintf(num_string, "%ld", number_int);

    // Append number to string
    string_t pan_str;
    string_init(pan_str);

    string_cat_printf(pan_str, "%s", num_string);
    free(num_string);

    //FURI_LOG_D("TAG", "Number: %s", num_string);
    //UNUSED(num_string);

    widget_add_string_element(
        nfc->widget, 64, 13, AlignCenter, AlignTop, FontSecondary, string_get_cstr(pan_str));
    string_clear(pan_str);

    // ----------------
    // CURSED CODE ENDS HERE
    // ----------------

    // Add last validator ID
    uint8_t last_id[2];
    memcpy(last_id, &troika_data->block[1 + (8 * 4 + 1)].value[0], 2);
    string_t last_id_str;
    string_init_printf(last_id_str, "VAL ID:%d", *last_id);
    widget_add_string_element(
        nfc->widget, 24, 32, AlignCenter, AlignTop, FontSecondary, string_get_cstr(last_id_str));
    string_clear(last_id_str);

    // Parse balance
    uint8_t balance_arr[] = {0x00, 0x00};
    memcpy(balance_arr, &troika_data->block[8 * 4 + 1].value[5], 2);
    uint16_t balance = 0;
    for(uint8_t i = 0; i < 2; i++) {
        balance <<= 8;
        balance |= balance_arr[i];
    }
    balance = balance / 25;
    FURI_LOG_D("TAG", "Balance: %d", balance);
    string_t disp_balance;
    string_init_printf(disp_balance, "Balance:%d", balance);
    widget_add_string_element(
        nfc->widget, 120, 23, AlignRight, AlignTop, FontSecondary, string_get_cstr(disp_balance));
    string_clear(disp_balance);
    char temp_str[32];

    // Add ATQA
    snprintf(temp_str, sizeof(temp_str), "ATQA: %02X%02X", nfc_data->atqa[0], nfc_data->atqa[1]);
    widget_add_string_element(nfc->widget, 121, 32, AlignRight, AlignTop, FontSecondary, temp_str);

    // Add UID
    snprintf(
        temp_str,
        sizeof(temp_str),
        "UID: %02X %02X %02X %02X",
        nfc_data->uid[0],
        nfc_data->uid[1],
        nfc_data->uid[2],
        nfc_data->uid[3]);
    widget_add_string_element(nfc->widget, 7, 42, AlignLeft, AlignTop, FontSecondary, temp_str);
    // Add SAK
    snprintf(temp_str, sizeof(temp_str), "SAK: %02X", nfc_data->sak);
    widget_add_string_element(nfc->widget, 121, 42, AlignRight, AlignTop, FontSecondary, temp_str);

    // Send notification
    if(scene_manager_get_scene_state(nfc->scene_manager, NfcSceneReadEmvDataSuccess) ==
       NFC_SEND_NOTIFICATION_TRUE) {
        notification_message(nfc->notifications, &sequence_success);
        scene_manager_set_scene_state(
            nfc->scene_manager, NfcSceneReadEmvDataSuccess, NFC_SEND_NOTIFICATION_FALSE);
    }

    view_dispatcher_switch_to_view(nfc->view_dispatcher, NfcViewWidget);
}

bool nfc_scene_troika_data_on_event(void* context, SceneManagerEvent event) {
    Nfc* nfc = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == GuiButtonTypeLeft) {
            consumed = scene_manager_search_and_switch_to_previous_scene(
                nfc->scene_manager, NfcSceneReadEmvAppSuccess);
        } else if(event.event == GuiButtonTypeRight) {
            // Clear device name
            nfc_device_set_name(nfc->dev, "");
            nfc->dev->format = NfcDeviceSaveFormatBankCard;
            scene_manager_next_scene(nfc->scene_manager, NfcSceneSaveName);
            consumed = true;
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        consumed = scene_manager_search_and_switch_to_previous_scene(
            nfc->scene_manager, NfcSceneReadEmvAppSuccess);
    }
    return consumed;
}

void nfc_scene_troika_data_on_exit(void* context) {
    Nfc* nfc = context;

    // Clear view
    widget_reset(nfc->widget);
}
