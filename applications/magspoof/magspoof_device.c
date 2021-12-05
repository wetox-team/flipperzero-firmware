#include "magspoof_device.h"

#include <lib/toolbox/path.h>
#include <lib/flipper_file/flipper_file.h>

static const char* magspoof_app_folder = "/any/magspoof";
static const char* magspoof_app_extension = ".magspoof";
// static const char* magspoof_app_shadow_extension = ".shd";
static const char* magspoof_file_header = "Flipper Magspoof device";
static const uint32_t magspoof_file_version = 1;

MagspoofDevice* magspoof_device_alloc() {
    MagspoofDevice* magspoof_dev = furi_alloc(sizeof(MagspoofDevice));
    magspoof_dev->storage = furi_record_open("storage");
    magspoof_dev->dialogs = furi_record_open("dialogs");
    string_init(magspoof_dev->data);
    return magspoof_dev;
}

void magspoof_device_free(MagspoofDevice* magspoof_dev) {
    furi_assert(magspoof_dev);
    furi_record_close("storage");
    furi_record_close("dialogs");
    free(magspoof_dev);
}

static bool magspoof_device_save_data(FlipperFile* file, MagspoofDevice* dev) {
    bool saved = false;
    // string_t* data = &(dev->data);
    // uint32_t data_temp = 0;

    do {
        if(!flipper_file_write_string(file, "Data", dev->data)) break;
        saved = true;
    } while(false);

    return saved;
}

bool magspoof_device_load_common_data(FlipperFile* file, MagspoofDevice* dev) {
    bool parsed = false;
    // string_t* data = &(dev->data);
    // memset(data, 0, sizeof(MagspoofDeviceCommonData));
    // uint32_t data_cnt = 0;
    // string_init(data);

    do {
        if(!flipper_file_read_string(file, "Data", dev->data)) break;
        // strlcpy(data->data, temp_str, sizeof(data->data));
        parsed = true;
    } while(false);

    // string_clear(temp_str);
    return parsed;
}

void magspoof_device_set_name(MagspoofDevice* dev, const char* name) {
    furi_assert(dev);

    strlcpy(dev->dev_name, name, MAGSPOOF_DEV_NAME_MAX_LEN);
}

static bool magspoof_device_save_file(
    MagspoofDevice* dev,
    const char* dev_name,
    const char* folder,
    const char* extension) {
    furi_assert(dev);

    bool saved = false;
    FlipperFile* file = flipper_file_alloc(dev->storage);
    // MagspoofDeviceCommonData* data = &dev->dev_data.magspoof_data;
    string_t temp_str;
    string_init(temp_str);

    do {
        // Create magspoof directory if necessary
        if(!storage_simply_mkdir(dev->storage, magspoof_app_folder)) break;
        // First remove magspoof device file if it was saved
        string_printf(temp_str, "%s/%s%s", folder, dev_name, extension);
        // Open file
        if(!flipper_file_open_always(file, string_get_cstr(temp_str))) break;
        // Write header
        if(!flipper_file_write_header_cstr(file, magspoof_file_header, magspoof_file_version)) break;
        
        if(!magspoof_device_save_data(file, dev)) break;
        
        saved = true;
    } while(0);

    if(!saved) {
        dialog_message_show_storage_error(dev->dialogs, "Can not save\nkey file");
    }
    string_clear(temp_str);
    flipper_file_close(file);
    flipper_file_free(file);
    return saved;
}

bool magspoof_device_save(MagspoofDevice* dev, const char* dev_name) {
    return magspoof_device_save_file(dev, dev_name, magspoof_app_folder, magspoof_app_extension);
}

static bool magspoof_device_load_data(MagspoofDevice* dev, string_t path) {
    bool parsed = false;
    FlipperFile* file = flipper_file_alloc(dev->storage);
    // MagspoofDeviceCommonData* data = &dev->dev_data.magspoof_data;
    // uint32_t data_cnt = 0;
    string_t temp_str;
    string_init(temp_str);
    bool depricated_version = false;

    do {
        if(!flipper_file_open_existing(file, string_get_cstr(path))) break;
        // Read and verify file header
        uint32_t version = 0;
        if(!flipper_file_read_header(file, temp_str, &version)) break;
        if(string_cmp_str(temp_str, magspoof_file_header) || (version != magspoof_file_version)) {
            depricated_version = true;
            break;
        }
    
        if(!magspoof_device_load_common_data(file, dev)) break;

        parsed = true;
    } while(false);

    if(!parsed) {
        if(depricated_version) {
            dialog_message_show_storage_error(dev->dialogs, "File format depricated");
        } else {
            dialog_message_show_storage_error(dev->dialogs, "Can not parse\nfile");
        }
    }

    string_clear(temp_str);
    flipper_file_close(file);
    flipper_file_free(file);
    return parsed;
}

bool magspoof_device_load(MagspoofDevice* dev, const char* file_path) {
    furi_assert(dev);
    furi_assert(file_path);

    // Load device data
    string_t path;
    string_init_set_str(path, file_path);
    bool dev_load = magspoof_device_load_data(dev, path);
    if(dev_load) {
        // Set device name
        path_extract_filename_no_ext(file_path, path);
        magspoof_device_set_name(dev, string_get_cstr(path));
    }
    string_clear(path);

    return dev_load;
}

bool magspoof_file_select(MagspoofDevice* dev) {
    furi_assert(dev);

    // Input events and views are managed by file_select
    bool res = dialog_file_select_show(
        dev->dialogs,
        magspoof_app_folder,
        magspoof_app_extension,
        dev->file_name,
        sizeof(dev->file_name),
        dev->dev_name);
    if(res) {
        string_t dev_str;
        // Get key file path
        string_init_printf(dev_str, "%s/%s%s", magspoof_app_folder, dev->file_name, magspoof_app_extension);
        res = magspoof_device_load_data(dev, dev_str);
        if(res) {
            magspoof_device_set_name(dev, dev->file_name);
        }
        string_clear(dev_str);
    }

    return res;
}


bool magspoof_device_delete(MagspoofDevice* dev) {
    furi_assert(dev);

    bool deleted = false;
    string_t file_path;
    string_init(file_path);

    do {
        // Delete original file
        string_init_printf(file_path, "%s/%s%s", magspoof_app_folder, dev->dev_name, magspoof_app_extension);
        if(!storage_simply_remove(dev->storage, string_get_cstr(file_path))) break;
        deleted = true;
    } while(0);

    if(!deleted) {
        dialog_message_show_storage_error(dev->dialogs, "Can not remove file");
    }

    string_clear(file_path);
    return deleted;
}
