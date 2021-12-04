//
// Created by kinjalik on 12/4/21.
//
#include <furi.h>
#include <ble.h>
#include "furi-hal-ohs.h"

#define TAG "FuriHalOhs"

#define CHECK_ERR(ret) \
    do {               \
        return false;  \
    } while(0)

bool furi_hal_ohs_stop() {
    FURI_LOG_I(TAG, "Stopping to advertize");
    printf( "Stopping to advertize\r\n");
    hci_le_set_advertise_enable(0x00);
    return true;
}

bool furi_hal_ohs_start() {
    uint8_t public_key[28] = {0xee, 0xf, 0x43, 0x34, 0xe4, 0x63, 0x1f, 0x22, 0x96, 0x8d, 0x56, 0xbc, 0x10, 0xe, 0xf8, 0x39, 0x50, 0xa6, 0x81, 0x8, 0xc7, 0x95, 0xf7, 0x92, 0x41, 0x84, 0x10, 0x6f};

    uint8_t rnd_addr[6] = {
        public_key[5],
        public_key[4],
        public_key[3],
        public_key[2],
        public_key[1],
        public_key[0] | (0b11 << 6),
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

    aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, rnd_addr);
    //    CHECK_ERR(ret);
    hci_le_set_advertising_parameters(0x0640, 0x0C80, 0x03, 0x00, 0x00, peer_addr, 0x07, 0x00);
    hci_le_set_advertising_data(31, adv_data);
    hci_le_set_advertise_enable(0x01);
    printf( "Started to advertize OHS with random addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
               rnd_addr[5],
               rnd_addr[4],
               rnd_addr[3],
               rnd_addr[2],
               rnd_addr[1],
               rnd_addr[0]
    );
    FURI_LOG_I(TAG, "Started to advertize OHS with random addr: %02x:%02x:%02x:%02x:%02x:%02x",
               rnd_addr[5],
               rnd_addr[4],
               rnd_addr[3],
               rnd_addr[2],
               rnd_addr[1],
               rnd_addr[0]
               );
    return true;
}
