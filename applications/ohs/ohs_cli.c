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


bool ohs_key_load(Ohs_key* ohs_key){
    ohs_key.key[28] = {0xf4, 0xa0, 0x49, 0xb5, 0x16, 0xe1, 0xa6, 0xfd, 0x1e, 0x4a,
                              0x86, 0x62, 0x98, 0x5b, 0xb4, 0x9a, 0x1f, 0x6d, 0xbe, 0x4e,
                              0xf4, 0xf3, 0x6a, 0xb,  0x39, 0x9b, 0xd9, 0x91};
    return true;
}

bool ohs_key_save(Ohs_key* ohs_key){
    return false;
}