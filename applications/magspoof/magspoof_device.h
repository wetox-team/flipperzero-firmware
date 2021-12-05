#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <m-string.h>
#include <storage/storage.h>
#include <dialogs/dialogs.h>

#include "mifare_ultralight.h"


#define MAGSPOOF_DEV_NAME_MAX_LEN 22
#define MAGSPOOF_FILE_NAME_MAX_LEN 120



typedef struct {
    string_t data;
} MagspoofDeviceCommonData;

typedef struct {
    MagspoofDeviceCommonData magspoof_data;
} MagspoofDeviceData;

typedef struct {
    Storage* storage;
    DialogsApp* dialogs;
    MagspoofDeviceData dev_data;
    string_t data;
    char dev_name[MAGSPOOF_DEV_NAME_MAX_LEN + 1];
    char file_name[MAGSPOOF_FILE_NAME_MAX_LEN];
    bool shadow_file_exist;
} MagspoofDevice;

MagspoofDevice* magspoof_device_alloc();

void magspoof_device_free(MagspoofDevice* magspoof_dev);

void magspoof_device_free_device_set_name(MagspoofDevice* dev, const char* name);

bool magspoof_device_save(MagspoofDevice* dev, const char* dev_name);

bool magspoof_device_load(MagspoofDevice* dev, const char* file_path);

bool magspoof_file_select(MagspoofDevice* dev);

void magspoof_device_clear(MagspoofDevice* dev);

bool magspoof_device_delete(MagspoofDevice* dev);
