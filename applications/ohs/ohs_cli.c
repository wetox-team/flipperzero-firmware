//
// Created by kinjalik on 12/4/21.
//

#include <furi.h>
#include <ble.h>
#include "furi-hal-ohs.h"
#include "ohs_cli.h"

void ohs_cli_init() {
    Cli* cli = furi_record_open("cli");
    cli_add_command(cli, "ohs_start", CliCommandFlagDefault, ohs_cli_command, NULL);
    cli_add_command(cli, "ohs_stop", CliCommandFlagDefault, ohs_cli_stop, NULL);
    cli_add_command(cli, "ohs_cli_key_save", CliCommandFlagDefault, ohs_cli_key_save, NULL);
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

void ohs_cli_key_save(Cli* cli, string_t args, void* context){
    if (args.sizeof() != 56){
        printf("Incorrect input\r\n");
        printf("Use 56-symbol hex\r\n");
    }
    else{
        uint8_t key[28];
        for (int i = 0; i < 28; i++){
            key[i] = fscanf(args+(i*2), "%02x", key[i]));
        }
        furi_hal_ohs_save_key(key, 28);
    }
}