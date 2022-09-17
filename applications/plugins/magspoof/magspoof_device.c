#include "magspoof_device.h"
#include <flipper_format/flipper_format.h>

#include <lib/toolbox/path.h>

#define MAGSPOOF_APP_FOLDER ANY_PATH("magspoof")
#define MAGSPOOF_APP_EXTENSION ".magspoof"

static const char* magspoof_file_header = "Flipper Magspoof device";
static const uint32_t magspoof_file_version = 1;

MagspoofDevice* magspoof_device_alloc() {
    MagspoofDevice* magspoof_dev = malloc(sizeof(MagspoofDevice));
    magspoof_dev->storage = furi_record_open("storage");
    magspoof_dev->dialogs = furi_record_open("dialogs");
    string_init(magspoof_dev->data);
    return magspoof_dev;
}

void magspoof_device_free(MagspoofDevice* magspoof_dev) {
    furi_assert(magspoof_dev);
    furi_record_close(RECORD_STORAGE);
    furi_record_close(RECORD_DIALOGS);
    free(magspoof_dev);
}

static bool magspoof_device_save_data(FlipperFormat* file, MagspoofDevice* dev) {
    bool saved = false;
    // string_t* data = &(dev->data);
    // uint32_t data_temp = 0;

    do {
        if(!flipper_format_write_string(file, "Data", dev->data)) break;
        saved = true;
    } while(false);

    return saved;
}

bool magspoof_device_load_common_data(FlipperFormat* file, MagspoofDevice* dev) {
    bool parsed = false;
    // string_t* data = &(dev->data);
    // memset(data, 0, sizeof(MagspoofDeviceCommonData));
    // uint32_t data_cnt = 0;
    // string_init(data);

    do {
        if(!flipper_format_read_string(file, "Data", dev->data)) break;
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
    const char* extension,
    bool use_load_path) {
    furi_assert(dev);

    bool saved = false;
    FlipperFormat* file = flipper_format_file_alloc(dev->storage);
    // MagspoofDeviceCommonData* data = &dev->dev_data.magspoof_data;
    string_t temp_str;
    string_init(temp_str);

    do {
        if(use_load_path && !string_empty_p(dev->load_path)) {
            // Get directory name
            path_extract_dirname(string_get_cstr(dev->load_path), temp_str);
            // Create magspoof directory if necessary
            if(!storage_simply_mkdir(dev->storage, string_get_cstr(temp_str))) break;
            // Make path to file to save
            string_cat_printf(temp_str, "/%s%s", dev_name, extension);
        } else {
            // Create magspoof directory if necessary
            if(!storage_simply_mkdir(dev->storage, MAGSPOOF_APP_FOLDER)) break;
            // First remove magspoof device file if it was saved
            string_printf(temp_str, "%s/%s%s", folder, dev_name, extension);
        }

        // Open file
        if(!flipper_format_file_open_always(file, string_get_cstr(temp_str))) break;
        // Write header
        if(!flipper_format_write_header_cstr(file, magspoof_file_header, magspoof_file_version)) break;
        
        if(!magspoof_device_save_data(file, dev)) break;
        
        saved = true;
    } while(0);

    if(!saved) {
        dialog_message_show_storage_error(dev->dialogs, "Can not save\nkey file");
    }
    string_clear(temp_str);
    flipper_format_file_close(file);
    flipper_format_free(file);
    return saved;
}

bool magspoof_device_save(MagspoofDevice* dev, const char* dev_name) {
    return magspoof_device_save_file(dev, dev_name, MAGSPOOF_APP_FOLDER, MAGSPOOF_APP_EXTENSION, true);
}

static bool magspoof_device_load_data(MagspoofDevice* dev, string_t path) {
    bool parsed = false;
    FlipperFormat* file = flipper_format_file_alloc(dev->storage);
    // MagspoofDeviceCommonData* data = &dev->dev_data.magspoof_data;
    // uint32_t data_cnt = 0;
    string_t temp_str;
    string_init(temp_str);
    bool depricated_version = false;

    do {
        if(!flipper_format_file_open_existing(file, string_get_cstr(path))) break;
        // Read and verify file header
        uint32_t version = 0;
        if(!flipper_format_read_header(file, temp_str, &version)) break;
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
    flipper_format_file_close(file);
    flipper_format_free(file);
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

    string_t magspoof_app_folder;
    string_init_set_str(magspoof_app_folder, MAGSPOOF_APP_FOLDER);

    DialogsFileBrowserOptions browser_options;
    dialog_file_browser_set_basic_options(&browser_options, MAGSPOOF_APP_EXTENSION, &I_unknown_10px);
    browser_options.hide_ext = false;

    bool res = dialog_file_browser_show(
        dev->dialogs,
        dev->load_path,
        magspoof_app_folder,
        &browser_options);

    if(res) {
        res = magspoof_device_load_data(dev, dev->load_path);
        if(res) {
            // Set device name
            string_t filename;
            string_init(filename);
            path_extract_filename(dev->load_path, filename, true);
            magspoof_device_set_name(dev, string_get_cstr(filename));
        }
    }

    return res;
}


bool magspoof_device_delete(MagspoofDevice* dev, bool use_load_path) {
    furi_assert(dev);

    bool deleted = false;
    string_t file_path;
    string_init(file_path);

    do {
        // Delete original file
        if(use_load_path && !string_empty_p(dev->load_path)) {
            string_set(file_path, dev->load_path);
        } else {
            string_printf(file_path, "%s/%s%s", MAGSPOOF_APP_FOLDER, dev->dev_name, MAGSPOOF_APP_EXTENSION);
        }
        if(!storage_simply_remove(dev->storage, string_get_cstr(file_path))) break;
        deleted = true;
    } while(0);

    if(!deleted) {
        dialog_message_show_storage_error(dev->dialogs, "Can not remove file");
    }

    string_clear(file_path);
    return deleted;
}
