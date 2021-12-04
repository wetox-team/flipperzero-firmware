#pragma once

#include <stdint.h>
#include <stdbool.h>

#define BT_SETTINGS_VERSION (2)

typedef enum {
    BT_MODE_OFF,
    BT_MODE_ON,
    BT_MODE_OHS
} BtMode;

typedef struct {
    uint8_t version;
    BtMode mode;
} BtSettings;

bool bt_settings_load(BtSettings* bt_settings);

bool bt_settings_save(BtSettings* bt_settings);
