//
// Created by kinjalik on 12/4/21.
//
#include <furi.h>
#include <ble.h>
#include "furi-hal-ohs.h"
#include <gap.h>
#include <file-worker.h>

#define TAG "FuriHalOhs"
#define OHS_KEY_PATH "/int/ohs.key"

#define CHECK_ERR(ret) \
    do {               \
        return false;  \
    } while(0)

bool furi_hal_ohs_stop() {
    FURI_LOG_I(TAG, "Stopping to advertize");
    printf("Stopping to advertize\r\n");
    hci_le_set_advertise_enable(0x00);
    return true;
}

bool furi_hal_ohs_start() {

    uint8_t ohs_key[28] = {};
    furi_hal_ohs_load_key(ohs_key);

    uint8_t rnd_addr[6] = {
        ohs_key[5],
        ohs_key[4],
        ohs_key[3],
        ohs_key[2],
        ohs_key[1],
        ohs_key[0] | (0b11 << 6),
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

    memcpy(&adv_data[7], &ohs_key[6], 22);
    adv_data[29] = ohs_key[0] >> 6;

    aci_hal_write_config_data(CONFIG_DATA_PUBADDR_OFFSET, CONFIG_DATA_PUBADDR_LEN, rnd_addr);
    //    CHECK_ERR(ret);
    hci_le_set_advertising_parameters(0x0640, 0x0C80, 0x03, 0x00, 0x00, peer_addr, 0x07, 0x00);
    hci_le_set_advertising_data(31, adv_data);
    hci_le_set_advertise_enable(0x01);
    printf("Started to advertize OHS with key: ");
    for (int i = 0; i < 28; i++) {
        printf(" %02x", ohs_key[i]);
    }
    printf(
        "\nStarted to advertize OHS with random addr: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
        rnd_addr[5],
        rnd_addr[4],
        rnd_addr[3],
        rnd_addr[2],
        rnd_addr[1],
        rnd_addr[0]);
    FURI_LOG_I(
        TAG,
        "Started to advertize OHS with random addr: %02x:%02x:%02x:%02x:%02x:%02x",
        rnd_addr[5],
        rnd_addr[4],
        rnd_addr[3],
        rnd_addr[2],
        rnd_addr[1],
        rnd_addr[0]);

    gap_notify_ohs_start();

    return true;
}

bool furi_hal_ohs_load_key(uint8_t* key) {
    furi_assert(key);
    bool file_loaded = false;
    uint8_t settings[28] = {};

    FURI_LOG_I(TAG, "Loading settings from \"%s\"", OHS_KEY_PATH);
    FileWorker* file_worker = file_worker_alloc(true);
    if(file_worker_open(file_worker, OHS_KEY_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        if(file_worker_read(file_worker, &settings, sizeof(settings))) {
            file_loaded = true;
        }
    }
    file_worker_free(file_worker);

    if(file_loaded) {
        FURI_LOG_I(TAG, "Settings load success");
        osKernelLock();
        memcpy(key, settings, 28);
        osKernelUnlock();
    } else {
        FURI_LOG_E(TAG, "Settings load failed");
    }
    return file_loaded;
}

bool furi_hal_ohs_save_key(uint8_t* key) {
    furi_assert(key);
    bool result = false;

    FileWorker* file_worker = file_worker_alloc(true);
    if(file_worker_open(file_worker, OHS_KEY_PATH, FSAM_WRITE, FSOM_OPEN_ALWAYS)) {
        if(file_worker_write(file_worker, key, 28)) {
            FURI_LOG_I(TAG, "Settings saved to \"%s\"", OHS_KEY_PATH);
            result = true;
        }
    }
    file_worker_free(file_worker);
    return result;
}