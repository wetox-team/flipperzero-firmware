#define _XOPEN_SOURCE
#include "nfc_supported_card.h"

#include <gui/modules/widget.h>
#include <nfc_worker_i.h>
#include <time.h>
#include <locale/locale.h>

#define TAG "Troyka parser"

static const MfClassicAuthContext troika_keys[] = {
    {.sector = 0, .key_a = 0xa0a1a2a3a4a5, .key_b = 0xfbf225dc5d58},
    {.sector = 1, .key_a = 0xa82607b01c0d, .key_b = 0x2910989b6880},
    {.sector = 2, .key_a = 0x2aa05ed1856f, .key_b = 0xeaac88e5dc99},
    {.sector = 3, .key_a = 0x2aa05ed1856f, .key_b = 0xeaac88e5dc99},
    {.sector = 4, .key_a = 0x73068f118c13, .key_b = 0x2b7f3253fac5},
    {.sector = 5, .key_a = 0xFBC2793D540B, .key_b = 0xd3a297dc2698},
    {.sector = 6, .key_a = 0x2aa05ed1856f, .key_b = 0xeaac88e5dc99},
    {.sector = 7, .key_a = 0xae3d65a3dad4, .key_b = 0x0f1c63013dbb},
    {.sector = 8, .key_a = 0xa73f5dc1d333, .key_b = 0xe35173494a81},
    {.sector = 9, .key_a = 0x69a32f1c2f19, .key_b = 0x6b8bd9860763},
    {.sector = 10, .key_a = 0x9becdf3d9273, .key_b = 0xf8493407799d},
    {.sector = 11, .key_a = 0x08b386463229, .key_b = 0x5efbaecef46b},
    {.sector = 12, .key_a = 0xcd4c61c26e3d, .key_b = 0x31c7610de3b0},
    {.sector = 13, .key_a = 0xa82607b01c0d, .key_b = 0x2910989b6880},
    {.sector = 14, .key_a = 0x0e8f64340ba4, .key_b = 0x4acec1205d75},
    {.sector = 15, .key_a = 0x2aa05ed1856f, .key_b = 0xeaac88e5dc99},
    {.sector = 16, .key_a = 0x6b02733bb6ec, .key_b = 0x7038cd25c408},
    {.sector = 17, .key_a = 0x403d706ba880, .key_b = 0xb39d19a280df},
    {.sector = 18, .key_a = 0xc11f4597efb5, .key_b = 0x70d901648cb9},
    {.sector = 19, .key_a = 0x0db520c78c1c, .key_b = 0x73e5b9d9d3a4},
    {.sector = 20, .key_a = 0x3ebce0925b2f, .key_b = 0x372cc880f216},
    {.sector = 21, .key_a = 0x16a27af45407, .key_b = 0x9868925175ba},
    {.sector = 22, .key_a = 0xaba208516740, .key_b = 0xce26ecb95252},
    {.sector = 23, .key_a = 0xCD64E567ABCD, .key_b = 0x8f79c4fd8a01},
    {.sector = 24, .key_a = 0x764cd061f1e6, .key_b = 0xa74332f74994},
    {.sector = 25, .key_a = 0x1cc219e9fec1, .key_b = 0xb90de525ceb6},
    {.sector = 26, .key_a = 0x2fe3cb83ea43, .key_b = 0xfba88f109b32},
    {.sector = 27, .key_a = 0x07894ffec1d6, .key_b = 0xefcb0e689db3},
    {.sector = 28, .key_a = 0x04c297b91308, .key_b = 0xc8454c154cb5},
    {.sector = 29, .key_a = 0x7a38e3511a38, .key_b = 0xab16584c972a},
    {.sector = 30, .key_a = 0x7545df809202, .key_b = 0xecf751084a80},
    {.sector = 31, .key_a = 0x5125974cd391, .key_b = 0xd3eafb5df46d},
    {.sector = 32, .key_a = 0x7a86aa203788, .key_b = 0xe41242278ca2},
    {.sector = 33, .key_a = 0xafcef64c9913, .key_b = 0x9db96dca4324},
    {.sector = 34, .key_a = 0x04eaa462f70b, .key_b = 0xac17b93e2fae},
    {.sector = 35, .key_a = 0xe734c210f27e, .key_b = 0x29ba8c3e9fda},
    {.sector = 36, .key_a = 0xd5524f591eed, .key_b = 0x5daf42861b4d},
    {.sector = 37, .key_a = 0xe4821a377b75, .key_b = 0xe8709e486465},
    {.sector = 38, .key_a = 0x518dc6eea089, .key_b = 0x97c64ac98ca4},
    {.sector = 39, .key_a = 0xbb52f8cce07f, .key_b = 0x6b6119752c70},
};

bool troika_parser_verify(NfcWorker* nfc_worker, FuriHalNfcTxRxContext* tx_rx) {
    furi_assert(nfc_worker);
    UNUSED(nfc_worker);
    if(nfc_worker->dev_data->mf_classic_data.type != MfClassicType1k &&
       nfc_worker->dev_data->mf_classic_data.type != MfClassicType4k) {
        return false;
    }

    uint8_t sector = 11;
    uint8_t block = mf_classic_get_sector_trailer_block_num_by_sector(sector);
    FURI_LOG_D("Troika", "Verifying sector %d", sector);
    if(mf_classic_authenticate(tx_rx, block, troika_keys[sector].key_a, MfClassicKeyA)) {
        FURI_LOG_D("Troika", "Sector %d verified", sector);
        return true;
    }
    return false;
}

bool troika_parser_read(NfcWorker* nfc_worker, FuriHalNfcTxRxContext* tx_rx) {
    furi_assert(nfc_worker);

    MfClassicData* mf_classic_data = &nfc_worker->dev_data->mf_classic_data;
    MfClassicType type = mf_classic_data->type;
    size_t sectors = type == MfClassicType4k ? 40 : 16;

    for(size_t i = 0; i < sectors; i++) {
        mf_classic_set_key_found(
            mf_classic_data, troika_keys[i].sector, MfClassicKeyA, troika_keys[i].key_a);
        mf_classic_set_key_found(
            mf_classic_data, troika_keys[i].sector, MfClassicKeyB, troika_keys[i].key_b);
    }

    uint8_t res = mf_classic_update_card(tx_rx, mf_classic_data);
    FURI_LOG_D("Troika", "Update card res: %d", res);
    return res == sectors;
}

bool troika_parser_parse(NfcDeviceData* dev_data) {
    MfClassicData* data = &dev_data->mf_classic_data;
    bool troika_parsed = false;

    do {
        // Verify key
        MfClassicSectorTrailer* sec_tr = mf_classic_get_sector_trailer_by_sector(data, 8);
        uint64_t key = nfc_util_bytes2num(sec_tr->key_a, 6);
        if(key != troika_keys[8].key_a) break;
        sec_tr = mf_classic_get_sector_trailer_by_sector(data, 4);
        key = nfc_util_bytes2num(sec_tr->key_a, 6);
        if(key != troika_keys[4].key_a) break;

        // Verify card type
        if(data->type != MfClassicType1k && data->type != MfClassicType4k) break;

        uint16_t transport_departament = data->block[32].value[0] << 8;
        transport_departament += data->block[32].value[1] >> 6;

        FURI_LOG_D(TAG, "Transport departament: %x", transport_departament);

        uint16_t layout_type = data->block[32].value[6];
        layout_type &= 0xffff;
        if(layout_type == 0xE) {
            layout_type = layout_type << 5;
            layout_type += (data->block[32].value[7]) >> 3;
        } else if(layout_type == 0xF) {
            layout_type = layout_type << 8;
            layout_type += (uint16_t)(data->block[32].value[7]);
            layout_type = layout_type << 2;
            layout_type += (uint16_t)(data->block[32].value[8]) >> 6;
        }

        FURI_LOG_D(TAG, "Layout type %x", layout_type);

        uint64_t card_number = 0;

        for(uint8_t i = 2; i < 7; ++i) {
            card_number <<= 8;
            card_number |= data->block[32].value[i];
        }
        card_number &= 0x0FFFFFFFFF;
        card_number >>= 4;

        uint32_t valid_to_days = 0;
        for(uint8_t i = 7; i < 10; ++i) {
            valid_to_days <<= 8;
            valid_to_days |= data->block[32].value[i];
        }
        valid_to_days >>= 6;
        valid_to_days &= 0xfff;
        time_t valid_to_seconds = 1546290000 + (valid_to_days - 1) * 24 * 60 * 60;
        struct tm valid_to_date;
        gmtime_r(&valid_to_seconds, &valid_to_date);
        // char valid_to_str[11];
        // strftime(valid_to_str, sizeof(valid_to_str), "%d.%m.%Y", valid_to_date);

        FURI_LOG_D(TAG, "Valid to (days): %ld", valid_to_days);

        uint64_t travel_time_minutes = 0;
        for(uint8_t i = 0; i < 3; ++i) {
            travel_time_minutes <<= 8;
            travel_time_minutes |= data->block[33].value[i];
        }
        travel_time_minutes >>= 1;
        // travel_time_minutes &= 0x3fffff;
        time_t travel_time_seconds = 1546290000 + travel_time_minutes * 60;
        struct tm travel_time;
        gmtime_r(&travel_time_seconds, &travel_time);
        // char travel_time_str[17];
        // strftime(travel_time_str, sizeof(travel_time_str), "%d.%m.%Y %H:%M", travel_time);

        FURI_LOG_D(TAG, "Travel from (minutes): %lld", travel_time_minutes);

        uint32_t balance = 0;
        for(uint8_t i = 5; i < 8; ++i) {
            balance <<= 8;
            balance |= data->block[33].value[i];
        }
        balance >>= 6;
        balance &= 0x1FFFF;

        balance /= 100;

        uint64_t validator_id = 0;
        for(uint8_t i = 7; i < 10; ++i) {
            validator_id <<= 8;
            validator_id |= data->block[33].value[i];
        }
        validator_id >>= 6;
        validator_id &= 0xFFFF;

        furi_string_printf(
            dev_data->parsed_data,
            "\e#Troika\nMetro card: %010lld\nValid for: %02d.%02d.%04d\nBalance: %ld rub\nCheck in from:\n%02d.%02d.%04d %02d:%02d\nValidator: %05lld",
            card_number,
            valid_to_date.tm_mday,
            valid_to_date.tm_mon,
            valid_to_date.tm_year,
            balance,
            travel_time.tm_mday,
            travel_time.tm_mon,
            travel_time.tm_year,
            travel_time.tm_hour,
            travel_time.tm_min,
            validator_id);
        // furi_string_printf(
        //     dev_data->parsed_data,
        //     "\e#Troika\nMetro card: %010lld\nValid for: %s\nBalance: %ld rub\nCheck in from:\n%s\nValidator: %05lld",
        //     card_number,
        //     asctime(valid_to_date),
        //     balance,
        //     asctime(travel_time),
        //     validator_id);
        troika_parsed = true;
    } while(false);

    return troika_parsed;
}
