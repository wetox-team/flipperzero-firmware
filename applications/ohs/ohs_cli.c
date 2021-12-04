//
// Created by kinjalik on 12/4/21.
//

#include <furi.h>
#include <furi-hal.h>
#include <ble.h>
#include "furi-hal-bt.h"
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

    uint8_t public_key[28] = {0x3a, 0x8f, 0x54, 0x1e, 0x2,  0x9c, 0x84, 0x75, 0xad, 0xef,
                              0x79, 0x1f, 0x4c, 0xb0, 0x7,  0xcf, 0xf2, 0xb1, 0xfb, 0x8f,
                              0xb0, 0xbe, 0x84, 0x13, 0xc8, 0x21, 0x4d, 0x4f};

    uint8_t rnd_addr[6] = {
        public_key[0] | (0b11 << 6),
        public_key[1],
        public_key[2],
        public_key[3],
        public_key[4],
        public_key[5],
    };

    uint8_t peer_addr[6] = {
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
        0x00,
    };

    uint8_t adv_data[31] = {
        0x1e, /* Pyaload length */
        0xff, /* Manufacturer Specific Data (type 0xff) */
        0x4c, 0x00, /* Company ID (Apple) */
        0x12, 0x19, /* Offline Finding type and length */
        0x00, /* State */
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* Public key */
        0x00, /* First two bits */
        0x00, /* Hint (0x00) */
    };

    memcpy(&adv_data[7], &public_key[6], 22);
    adv_data[29] = public_key[0] >> 6;

    tBleStatus ret;
    ret =
        hci_le_set_advertising_parameters(0x0640, 0x0C80, 0x03, 0x01, 0x00, peer_addr, 0x07, 0x00);
    CHECK_ERR(ret);
    ret = hci_le_set_advertising_data(31, adv_data);
    CHECK_ERR(ret);
    ret = hci_le_set_random_address(rnd_addr);
    CHECK_ERR(ret);
    ret = hci_le_set_advertise_enable(0x01);
    CHECK_ERR(ret);

    printf("Random addr: ");
    for(int i = 0; i < 6; i++) {
        printf("%x ", rnd_addr[i]);
    }
    printf("\r\n");
}