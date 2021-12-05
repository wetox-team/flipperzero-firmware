//
// Created by kinjalik on 12/4/21.
//

#include <furi.h>
#include <ble.h>
#include "furi-hal-ohs.h"
#include "ohs_cli.h"

void ohs_cli_init() {
    Cli* cli = furi_record_open("cli");
//    cli_add_command(cli, "ohs_start", CliCommandFlagDefault, ohs_cli_command, NULL);
//    cli_add_command(cli, "ohs_stop", CliCommandFlagDefault, ohs_cli_stop, NULL);
    cli_add_command(cli, "ohs_enter_key", CliCommandFlagDefault, ohs_cli_key_enter, NULL);
    cli_add_command(cli, "ohs_print_key", CliCommandFlagDefault, ohs_cli_key_print, NULL);
    cli_add_command(cli, "ohs_print_mac_addr", CliCommandFlagDefault, ohs_cli_mac_print, NULL);
    furi_record_close("cli");
}

#define CHECK_ERR(ret)                                                                     \
    do {                                                                                   \
        if(ret != BLE_STATUS_SUCCESS) printf("Error: 0x%X on line %d\r\n", ret, __LINE__); \
    } while(0)

void ohs_cli_stop(Cli* cli, string_t args, void* context) {
    printf("Stopping advertising, kinda\r\n");

    tBleStatus ret;
    ret = hci_le_set_advertise_enable(0x00);
    CHECK_ERR(ret);
}

void ohs_cli_command(Cli* cli, string_t args, void* context) {
    printf("Starting advertising, kinda\r\n");

    furi_hal_ohs_start();
}

void ohs_cli_key_enter(Cli* cli, string_t args, void* context){
    if (string_size(args) != 56){
        printf("Incorrect input. Save aborted.\r\n");
        printf("Use 56-symbol hex\r\n");
    }
    else {
        uint8_t key[28];

        for(size_t i = 0; i < 28; i++) {
            if(sscanf(string_get_cstr(args) + i * 2, "%02x", (unsigned int*)&key[i]) != 1) {
                printf("Something went wrong with this key\r\n");
                return;
            }
        }
        furi_hal_ohs_save_key(key);
    }
}

void ohs_cli_key_print(Cli* cli, string_t args, void* context){
    uint8_t ohs_key[28] = {};
    furi_hal_ohs_load_key(ohs_key);

    printf("Currently saved key in %s: ", OHS_KEY_PATH);
    for (int i = 0; i < 28; i++){
        printf("%02x ", ohs_key[i]);
    }
    printf("\r\n");
}

void ohs_cli_mac_print(Cli* cli, string_t args, void* context) {
    uint8_t mac_addr[6] = {};
    furi_assert(mac_addr != NULL);
    printf("DO furi_hal_ohs_get_mac\r\n");
    furi_hal_ohs_get_mac(mac_addr);
    printf("POSLE furi_hal_ohs_get_mac\r\n");
    furi_assert(mac_addr != NULL);
    for (int i = 0; i < 6; i++) {
        printf("%02x", (unsigned int) mac_addr[i]);
        if (i != 5)
            printf(":");
    }
}