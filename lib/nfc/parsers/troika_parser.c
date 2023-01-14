#include "nfc_supported_card.h"

#include <gui/modules/widget.h>
#include <nfc_worker_i.h>
#include <furi_hal_rtc.h>
#include <nfc/helpers/bit_lib.h>

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

void from_days_to_datetime(uint16_t days, FuriHalRtcDateTime* datetime, uint16_t start_year) {
    uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    datetime->day = (days - 1) % 365 - 1;
    for(uint8_t i = 0; i < 12; ++i) {
        datetime->month++;
        if(datetime->day < days_in_month[i]) {
            break;
        } else {
            datetime->day -= days_in_month[i];
        }
    }
    datetime->year = (days - 1) / 365 + start_year;
}

void from_minutes_to_datetime(uint32_t minutes, FuriHalRtcDateTime* datetime, uint16_t start_year) {
    datetime->minute = minutes % 60;
    datetime->hour = minutes / 60 % 24;
    from_days_to_datetime(minutes / 60 / 24, datetime, start_year);
    datetime->day += 1;
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

        uint16_t transport_departament = bit_lib_get_bits_16(data->block[32].value, 0, 10);

        FURI_LOG_D(TAG, "Transport departament: %x", transport_departament);

        uint16_t layout_type = bit_lib_get_bits_16(data->block[32].value, 52, 4);
        if(layout_type == 0xE) {
            layout_type = bit_lib_get_bits_16(data->block[32].value, 52, 9);
        } else if(layout_type == 0xF) {
            layout_type = bit_lib_get_bits_16(data->block[32].value, 52, 14);
        }

        FURI_LOG_D(TAG, "Layout type %x", layout_type);

        uint16_t card_view = 0;
        uint16_t card_type = 0;
        uint32_t card_number = 0;
        uint8_t card_layout = 0;
        uint8_t card_layout2 = 0;
        uint16_t card_use_before_date = 0;
        uint16_t card_blank_type = 0;
        uint32_t card_start_trip_minutes = 0;
        uint8_t card_minutes_pass = 0;
        uint32_t card_remaining_funds = 0;
        uint16_t card_validator = 0;
        uint8_t card_blocked = 0;
        uint32_t card_hash = 0;
        FuriString* result = furi_string_alloc();

        switch(layout_type) {
        case 0x02: {
            card_view = bit_lib_get_bits_16(data->block[32].value, 0, 10); //101
            card_type = bit_lib_get_bits_16(data->block[32].value, 10, 10); //102
            card_number = bit_lib_get_bits_32(data->block[32].value, 20, 32); //201
            card_layout = bit_lib_get_bits(data->block[32].value, 52, 4); //111
            card_use_before_date = bit_lib_get_bits_16(data->block[32].value, 56, 16); //202
            uint8_t card_benefit_code = bit_lib_get_bits(data->block[32].value, 72, 8); //124
            uint32_t card_rfu1 = bit_lib_get_bits(data->block[32].value, 80, 32); //rfu1
            uint16_t card_crc16 = bit_lib_get_bits_16(data->block[32].value, 112, 16); //501.1
            card_blocked = bit_lib_get_bits(data->block[32].value, 128, 1); //303
            uint16_t card_start_trip_time =
                bit_lib_get_bits_16(data->block[32].value, 177, 12); //403
            uint16_t card_start_trip_date =
                bit_lib_get_bits_16(data->block[32].value, 189, 16); //402
            uint16_t card_valid_from_date =
                bit_lib_get_bits_16(data->block[32].value, 157, 16); //311
            uint16_t card_valid_by_date =
                bit_lib_get_bits_16(data->block[32].value, 173, 16); //312
            uint8_t card_start_trip_seconds =
                bit_lib_get_bits(data->block[32].value, 189, 6); //406
            uint8_t card_transport_type1 = bit_lib_get_bits(data->block[32].value, 180, 2); //421.1
            uint8_t card_transport_type2 = bit_lib_get_bits(data->block[32].value, 182, 2); //421.2
            uint8_t card_transport_type3 = bit_lib_get_bits(data->block[32].value, 184, 2); //421.3
            uint8_t card_transport_type4 = bit_lib_get_bits(data->block[32].value, 186, 2); //421.4
            uint16_t card_use_with_date =
                bit_lib_get_bits_16(data->block[32].value, 189, 16); //205
            uint8_t card_route = bit_lib_get_bits(data->block[32].value, 205, 1); //424
            card_remaining_funds = bit_lib_get_bits_32(data->block[32].value, 188, 22); //322
            card_hash = bit_lib_get_bits_32(data->block[32].value, 224, 32); //502
            uint16_t card_validator1 = bit_lib_get_bits_16(data->block[32].value, 206, 15); //422.1
            card_validator = bit_lib_get_bits_16(data->block[32].value, 205, 16); //422
            uint16_t card_total_trips = bit_lib_get_bits_16(data->block[32].value, 221, 16); //331
            uint8_t card_write_enabled =
                bit_lib_get_bits(data->block[32].value, 237, 1); //write_enabled
            uint8_t card_rfu2 = bit_lib_get_bits(data->block[32].value, 238, 2); //rfu2
            uint16_t card_crc16_2 = bit_lib_get_bits_16(data->block[32].value, 240, 16); //501.2

            FURI_LOG_D(
                TAG,
                "%x %x %lx %x %x %lx %x %x %x %x %x %x %x %x %x %x %x %x %x %lx %lx %x %x %x %x %x %x",
                card_view,
                card_type,
                card_number,
                card_use_before_date,
                card_benefit_code,
                card_rfu1,
                card_crc16,
                card_blocked,
                card_start_trip_time,
                card_start_trip_date,
                card_valid_from_date,
                card_valid_by_date,
                card_start_trip_seconds,
                card_transport_type1,
                card_transport_type2,
                card_transport_type3,
                card_transport_type4,
                card_use_with_date,
                card_route,
                card_remaining_funds,
                card_hash,
                card_validator1,
                card_validator,
                card_total_trips,
                card_write_enabled,
                card_rfu2,
                card_crc16_2);
            break;
        }
        case 0x06: {
            card_view = bit_lib_get_bits_16(data->block[32].value, 0, 10); //101
            card_type = bit_lib_get_bits_16(data->block[32].value, 10, 10); //102
            card_number = bit_lib_get_bits_32(data->block[32].value, 20, 32); //201
            card_layout = bit_lib_get_bits(data->block[32].value, 52, 4); //111
            card_use_before_date = bit_lib_get_bits_16(data->block[32].value, 56, 16); //202
            uint8_t card_geozone_a = bit_lib_get_bits(data->block[32].value, 72, 4); //GeoZoneA
            uint8_t card_geozone_b = bit_lib_get_bits(data->block[32].value, 76, 4); //GeoZoneB
            card_blank_type = bit_lib_get_bits_16(data->block[32].value, 80, 10); //121.
            uint16_t card_type_of_extended =
                bit_lib_get_bits_16(data->block[32].value, 90, 10); //122
            uint32_t card_rfu1 = bit_lib_get_bits(data->block[32].value, 100, 12); //rfu1
            uint16_t card_crc16 = bit_lib_get_bits_16(data->block[32].value, 112, 16); //501.1
            card_blocked = bit_lib_get_bits(data->block[32].value, 128, 1); //303
            uint16_t card_start_trip_time =
                bit_lib_get_bits_16(data->block[32].value, 129, 12); //403
            uint16_t card_start_trip_date =
                bit_lib_get_bits_16(data->block[32].value, 141, 16); //402
            uint16_t card_valid_from_date =
                bit_lib_get_bits_16(data->block[32].value, 157, 16); //311
            uint16_t card_valid_by_date =
                bit_lib_get_bits_16(data->block[32].value, 173, 16); //312
            uint16_t card_company = bit_lib_get_bits(data->block[32].value, 189, 4); //Company
            uint8_t card_validator1 = bit_lib_get_bits(data->block[32].value, 193, 2); //422.1
            uint16_t card_remaining_trips =
                bit_lib_get_bits_16(data->block[32].value, 197, 10); //321
            uint8_t card_units = bit_lib_get_bits(data->block[32].value, 207, 6); //Units
            uint16_t card_validator2 = bit_lib_get_bits_16(data->block[32].value, 213, 10); //422.2
            uint16_t card_total_trips = bit_lib_get_bits_16(data->block[32].value, 221, 16); //331
            uint8_t card_extended = bit_lib_get_bits(data->block[32].value, 239, 1); //123
            uint16_t card_crc16_2 = bit_lib_get_bits_16(data->block[32].value, 240, 16); //501.2

            FURI_LOG_D(
                TAG,
                "%x %x %lx %x %x %x %x %x %lx %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
                card_view,
                card_type,
                card_number,
                card_use_before_date,
                card_geozone_a,
                card_geozone_b,
                card_blank_type,
                card_type_of_extended,
                card_rfu1,
                card_crc16,
                card_blocked,
                card_start_trip_time,
                card_start_trip_date,
                card_valid_from_date,
                card_valid_by_date,
                card_company,
                card_validator1,
                card_remaining_trips,
                card_units,
                card_validator2,
                card_total_trips,
                card_extended,
                card_crc16_2);
            FuriHalRtcDateTime card_use_before_date_s = {0};
            from_days_to_datetime(card_use_before_date, &card_use_before_date_s, 1992);

            FuriHalRtcDateTime card_start_trip_minutes_s = {0};
            from_minutes_to_datetime(
                card_start_trip_date * 24 * 60 + card_start_trip_time,
                &card_start_trip_minutes_s,
                1992);
            furi_string_printf(
                result,
                "Number: %ld\nValid for: %02d.%02d.%04d\nTrips left: %d of %d\nTrip from: %02d.%02d.%04d %02d:%02d\nValidator: %05d",
                card_number,
                card_use_before_date_s.day,
                card_use_before_date_s.month,
                card_use_before_date_s.year,
                card_remaining_trips,
                card_total_trips,
                card_start_trip_minutes_s.day,
                card_start_trip_minutes_s.month,
                card_start_trip_minutes_s.year,
                card_start_trip_minutes_s.hour,
                card_start_trip_minutes_s.minute,
                card_validator);
            break;
        }
        case 0x08: {
            card_view = bit_lib_get_bits_16(data->block[32].value, 0, 10); //101
            card_type = bit_lib_get_bits_16(data->block[32].value, 10, 10); //102
            card_number = bit_lib_get_bits_32(data->block[32].value, 20, 32); //201
            card_layout = bit_lib_get_bits(data->block[32].value, 52, 4); //111
            card_use_before_date = bit_lib_get_bits_16(data->block[32].value, 56, 16); //202
            // uint64_t card_rfu1 = bit_lib_get_bits(data->block[32].value, 72, 56); //rfu1
            uint16_t card_valid_from_date =
                bit_lib_get_bits_16(data->block[32].value, 128, 16); //311
            uint8_t card_valid_for_days = bit_lib_get_bits(data->block[32].value, 144, 8); //313
            uint8_t card_requires_activation =
                bit_lib_get_bits(data->block[32].value, 152, 1); //301
            uint8_t card_rfu2 = bit_lib_get_bits(data->block[32].value, 153, 7); //rfu2
            uint16_t card_remaining_trips1 =
                bit_lib_get_bits_16(data->block[32].value, 160, 8); //321.1
            uint16_t card_remaining_trips =
                bit_lib_get_bits_16(data->block[32].value, 168, 8); //321
            uint8_t card_validator1 = bit_lib_get_bits(data->block[32].value, 193, 2); //422.1
            uint16_t card_validator = bit_lib_get_bits_16(data->block[32].value, 177, 15); //422
            card_hash = bit_lib_get_bits_32(data->block[32].value, 192, 32); //502
            uint32_t card_rfu3 = bit_lib_get_bits_32(data->block[32].value, 224, 32); //rfu3

            FURI_LOG_D(
                TAG,
                "%x %x %lx %x %x %x %x %x %x %x %x %x %lx %x %lx",
                card_view,
                card_type,
                card_number,
                card_use_before_date,
                card_valid_from_date,
                card_valid_for_days,
                card_requires_activation,
                card_rfu2,
                card_remaining_trips1,
                card_remaining_trips,
                card_validator1,
                card_validator,
                card_hash,
                card_valid_from_date,
                card_rfu3);
            FuriHalRtcDateTime card_use_before_date_s = {0};
            from_days_to_datetime(card_use_before_date, &card_use_before_date_s, 1992);

            furi_string_printf(
                result,
                "Number: %ld\nValid for: %02d.%02d.%04d\nTrips left: %d\nValidator: %05d",
                card_number,
                card_use_before_date_s.day,
                card_use_before_date_s.month,
                card_use_before_date_s.year,
                card_remaining_trips,
                card_validator);
            break;
        }
        case 0x0A: {
            card_view = bit_lib_get_bits_16(data->block[32].value, 0, 10); //101
            card_type = bit_lib_get_bits_16(data->block[32].value, 10, 10); //102
            card_number = bit_lib_get_bits_32(data->block[32].value, 20, 32); //201
            card_layout = bit_lib_get_bits(data->block[32].value, 52, 4); //111
            uint16_t card_valid_from_date =
                bit_lib_get_bits_16(data->block[32].value, 128, 16); //311
            uint32_t card_valid_for_minutes =
                bit_lib_get_bits_32(data->block[32].value, 76, 19); //314
            uint8_t card_requires_activation =
                bit_lib_get_bits(data->block[32].value, 95, 1); //301
            card_start_trip_minutes = bit_lib_get_bits_32(data->block[32].value, 96, 19); //405
            card_minutes_pass = bit_lib_get_bits(data->block[32].value, 119, 7); //412
            uint8_t card_transport_type_flag =
                bit_lib_get_bits(data->block[32].value, 126, 2); //421.0
            uint16_t card_remaining_trips =
                bit_lib_get_bits_16(data->block[32].value, 128, 8); //321
            uint16_t card_validator = bit_lib_get_bits_16(data->block[32].value, 136, 16); //422
            uint8_t card_transport_type1 = bit_lib_get_bits(data->block[32].value, 152, 2); //421.1
            uint8_t card_transport_type2 = bit_lib_get_bits(data->block[32].value, 154, 2); //421.2
            uint8_t card_transport_type3 = bit_lib_get_bits(data->block[32].value, 156, 2); //421.3
            uint8_t card_transport_type4 = bit_lib_get_bits(data->block[32].value, 158, 2); //421.4
            card_hash = bit_lib_get_bits_32(data->block[32].value, 192, 32); //502

            FURI_LOG_D(
                TAG,
                "%x %x %lx %x %x %lx %x %lx %x %x %x %x %x %x %x %x %lx",
                card_view,
                card_type,
                card_number,
                card_use_before_date,
                card_valid_from_date,
                card_valid_for_minutes,
                card_requires_activation,
                card_start_trip_minutes,
                card_minutes_pass,
                card_transport_type_flag,
                card_remaining_trips,
                card_validator,
                card_transport_type1,
                card_transport_type2,
                card_transport_type3,
                card_transport_type4,
                card_hash);
            FuriHalRtcDateTime card_use_before_date_s = {0};
            from_days_to_datetime(card_use_before_date, &card_use_before_date_s, 2016);

            FuriHalRtcDateTime card_start_trip_minutes_s = {0};
            from_minutes_to_datetime(card_start_trip_minutes, &card_start_trip_minutes_s, 2016);
            furi_string_printf(
                result,
                "Number: %ld\nValid for: %02d.%02d.%04d\nTrip from: %02d.%02d.%04d %02d:%02d\nTrips left: %d\nValidator: %05d",
                card_number,
                card_use_before_date_s.day,
                card_use_before_date_s.month,
                card_use_before_date_s.year,
                card_start_trip_minutes_s.day,
                card_start_trip_minutes_s.month,
                card_start_trip_minutes_s.year,
                card_start_trip_minutes_s.hour,
                card_start_trip_minutes_s.minute,
                card_remaining_trips,
                card_validator);
            break;
        }
        case 0x0C: {
            card_view = bit_lib_get_bits_16(data->block[32].value, 0, 10); //101
            card_type = bit_lib_get_bits_16(data->block[32].value, 10, 10); //102
            card_number = bit_lib_get_bits_32(data->block[32].value, 20, 32); //201
            card_layout = bit_lib_get_bits(data->block[32].value, 52, 4); //111
            card_use_before_date = bit_lib_get_bits_16(data->block[32].value, 56, 16); //202
            // uint64_t card_rfu1 = bit_lib_get_bits(data->block[32].value, 72, 56); //rfu1
            uint16_t card_valid_from_date =
                bit_lib_get_bits_16(data->block[32].value, 128, 16); //311
            uint8_t card_valid_for_days = bit_lib_get_bits(data->block[32].value, 144, 8); //313
            uint8_t card_requires_activation =
                bit_lib_get_bits(data->block[32].value, 152, 1); //301
            uint8_t card_rfu2 = bit_lib_get_bits(data->block[32].value, 153, 13); //rfu2
            uint16_t card_remaining_trips =
                bit_lib_get_bits_16(data->block[32].value, 166, 10); //321
            uint16_t card_validator = bit_lib_get_bits_16(data->block[32].value, 176, 16); //422
            card_hash = bit_lib_get_bits_32(data->block[32].value, 192, 32); //502
            uint16_t card_start_trip_date =
                bit_lib_get_bits_16(data->block[32].value, 224, 16); //402
            uint16_t card_start_trip_time =
                bit_lib_get_bits_16(data->block[32].value, 240, 11); //403
            uint8_t card_transport_type = bit_lib_get_bits(data->block[32].value, 251, 2); //421
            uint32_t card_rfu3 = bit_lib_get_bits_32(data->block[32].value, 253, 2); //rfu3
            uint8_t card_transfer_in_metro = bit_lib_get_bits(data->block[32].value, 255, 1); //432

            FURI_LOG_D(
                TAG,
                "%x %x %lx %x %x %x %x %x %x %x %x %x %x %lx %x",
                card_view,
                card_type,
                card_number,
                card_use_before_date,
                card_valid_from_date,
                card_valid_for_days,
                card_requires_activation,
                card_rfu2,
                card_remaining_trips,
                card_validator,
                card_start_trip_date,
                card_start_trip_time,
                card_transport_type,
                card_rfu3,
                card_transfer_in_metro);
            FuriHalRtcDateTime card_use_before_date_s = {0};
            from_days_to_datetime(card_use_before_date, &card_use_before_date_s, 1992);
            FuriHalRtcDateTime card_start_trip_minutes_s = {0};
            from_minutes_to_datetime(
                card_start_trip_date * 24 * 60 + card_start_trip_time,
                &card_start_trip_minutes_s,
                1992);
            furi_string_printf(
                result,
                "Number: %010ld\nValid for: %02d.%02d.%04d\nTrip from: %02d.%02d.%04d %02d:%02d\nTrips left: %d\nValidator: %05d",
                card_number,
                card_use_before_date_s.day,
                card_use_before_date_s.month,
                card_use_before_date_s.year,
                card_start_trip_minutes_s.day,
                card_start_trip_minutes_s.month,
                card_start_trip_minutes_s.year,
                card_start_trip_minutes_s.hour,
                card_start_trip_minutes_s.minute,
                card_remaining_trips,
                card_validator);
            break;
        }
        case 0x0D: {
            card_view = bit_lib_get_bits_16(data->block[32].value, 0, 10); //101
            card_type = bit_lib_get_bits_16(data->block[32].value, 10, 10); //102
            card_number = bit_lib_get_bits_32(data->block[32].value, 20, 32); //201
            card_layout = bit_lib_get_bits(data->block[32].value, 52, 4); //111
            uint8_t card_rfu1 = bit_lib_get_bits(data->block[32].value, 56, 8); //rfu1
            card_use_before_date = bit_lib_get_bits_16(data->block[32].value, 64, 16); //202
            uint16_t card_valid_for_time =
                bit_lib_get_bits_16(data->block[32].value, 80, 11); //316
            uint8_t card_rfu2 = bit_lib_get_bits(data->block[32].value, 91, 5); //rfu2
            uint16_t card_use_before_date2 =
                bit_lib_get_bits_16(data->block[32].value, 96, 16); //202.2
            uint16_t card_valid_for_time2 =
                bit_lib_get_bits_16(data->block[32].value, 123, 11); //316.2
            uint8_t card_rfu3 = bit_lib_get_bits(data->block[32].value, 123, 5); //rfu3
            uint16_t card_valid_from_date =
                bit_lib_get_bits_16(data->block[32].value, 128, 16); //311
            uint8_t card_valid_for_days = bit_lib_get_bits(data->block[32].value, 144, 8); //313
            uint8_t card_requires_activation =
                bit_lib_get_bits(data->block[32].value, 152, 1); //301
            uint8_t card_rfu4 = bit_lib_get_bits(data->block[32].value, 153, 2); //rfu4
            uint8_t card_passage_5_minutes = bit_lib_get_bits(data->block[32].value, 155, 5); //413
            uint8_t card_transport_type1 = bit_lib_get_bits(data->block[32].value, 160, 2); //421.1
            uint8_t card_passage_in_metro = bit_lib_get_bits(data->block[32].value, 162, 1); //431
            uint8_t card_passages_ground_transport =
                bit_lib_get_bits(data->block[32].value, 163, 3); //433
            uint16_t card_remaining_trips =
                bit_lib_get_bits_16(data->block[32].value, 166, 10); //321
            uint16_t card_validator = bit_lib_get_bits_16(data->block[32].value, 176, 16); //422
            card_hash = bit_lib_get_bits_32(data->block[32].value, 192, 32); //502
            uint16_t card_start_trip_date =
                bit_lib_get_bits_16(data->block[32].value, 224, 16); //402
            uint16_t card_start_trip_time =
                bit_lib_get_bits_16(data->block[32].value, 240, 11); //403
            uint8_t card_transport_type2 = bit_lib_get_bits(data->block[32].value, 251, 2); //421.2
            uint8_t card_rfu5 = bit_lib_get_bits(data->block[32].value, 253, 2); //rfu5
            uint8_t card_transfer_in_metro = bit_lib_get_bits(data->block[32].value, 255, 1); //432

            FURI_LOG_D(
                TAG,
                "%x %x %lx %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
                card_view,
                card_type,
                card_number,
                card_layout,
                card_rfu1,
                card_use_before_date,
                card_valid_for_time,
                card_rfu2,
                card_use_before_date2,
                card_valid_for_time2,
                card_rfu3,
                card_valid_from_date,
                card_valid_for_days,
                card_requires_activation,
                card_rfu4,
                card_passage_5_minutes,
                card_transport_type1,
                card_passage_in_metro,
                card_passages_ground_transport,
                card_remaining_trips,
                card_validator,
                card_start_trip_date,
                card_start_trip_time,
                card_transport_type2,
                card_rfu5,
                card_transfer_in_metro);
            FuriHalRtcDateTime card_use_before_date_s = {0};
            from_days_to_datetime(card_use_before_date, &card_use_before_date_s, 1992);
            FuriHalRtcDateTime card_start_trip_minutes_s = {0};
            from_minutes_to_datetime(
                card_start_trip_date * 24 * 60 + card_start_trip_time,
                &card_start_trip_minutes_s,
                1992);
            furi_string_printf(
                result,
                "Number: %010ld\nValid for: %02d.%02d.%04d\nTrip from: %02d.%02d.%04d %02d:%02d\nTrips left: %d\nValidator: %05d",
                card_number,
                card_use_before_date_s.day,
                card_use_before_date_s.month,
                card_use_before_date_s.year,
                card_start_trip_minutes_s.day,
                card_start_trip_minutes_s.month,
                card_start_trip_minutes_s.year,
                card_start_trip_minutes_s.hour,
                card_start_trip_minutes_s.minute,
                card_remaining_trips,
                card_validator);
            break;
        }
        case 0x1C1: {
            card_view = bit_lib_get_bits_16(data->block[32].value, 0, 10); //101
            card_type = bit_lib_get_bits_16(data->block[32].value, 10, 10); //102
            card_number = bit_lib_get_bits_32(data->block[32].value, 20, 32); //201
            card_layout = bit_lib_get_bits(data->block[32].value, 52, 4); //111
            card_layout2 = bit_lib_get_bits(data->block[32].value, 56, 5); //112
            card_use_before_date = bit_lib_get_bits_16(data->block[32].value, 61, 16); //202.
            card_blank_type = bit_lib_get_bits_16(data->block[32].value, 77, 10); //121.
            card_validator = bit_lib_get_bits_16(data->block[32].value, 128, 16); //422
            uint16_t card_start_trip_date =
                bit_lib_get_bits_16(data->block[32].value, 144, 16); //402
            uint16_t card_start_trip_time =
                bit_lib_get_bits_16(data->block[32].value, 160, 11); //403
            uint8_t card_transport_type1 = bit_lib_get_bits(data->block[32].value, 171, 2); //421.1
            uint8_t card_transport_type2 = bit_lib_get_bits(data->block[32].value, 173, 2); //421.2
            uint8_t card_transfer_in_metro = bit_lib_get_bits(data->block[32].value, 177, 1); //432
            uint8_t card_passage_in_metro = bit_lib_get_bits(data->block[32].value, 178, 1); //431
            uint8_t card_passages_ground_transport =
                bit_lib_get_bits(data->block[32].value, 179, 3); //433
            card_minutes_pass = bit_lib_get_bits(data->block[32].value, 185, 8); //412.
            card_remaining_funds = bit_lib_get_bits_32(data->block[32].value, 196, 19); //322
            uint8_t card_fare_trip = bit_lib_get_bits(data->block[32].value, 215, 2); //441
            card_blocked = bit_lib_get_bits(data->block[32].value, 202, 1); //303
            uint8_t card_zoo = bit_lib_get_bits(data->block[32].value, 218, 1); //zoo
            card_hash = bit_lib_get_bits_32(data->block[32].value, 224, 32); //502

            FURI_LOG_D(
                TAG,
                "%x %x %lx %x %x %x %x %x %x %x %x %x %x %x %x %x %lx %x %x %x %lx",
                card_view,
                card_type,
                card_number,
                card_layout,
                card_layout2,
                card_use_before_date,
                card_blank_type,
                card_validator,
                card_start_trip_date,
                card_start_trip_time,
                card_transport_type1,
                card_transport_type2,
                card_transfer_in_metro,
                card_passage_in_metro,
                card_passages_ground_transport,
                card_minutes_pass,
                card_remaining_funds,
                card_fare_trip,
                card_blocked,
                card_zoo,
                card_hash);
            FuriHalRtcDateTime card_use_before_date_s = {0};
            from_days_to_datetime(card_use_before_date, &card_use_before_date_s, 1992);

            FuriHalRtcDateTime card_start_trip_minutes_s = {0};
            from_minutes_to_datetime(card_start_trip_minutes, &card_start_trip_minutes_s, 1992);
            furi_string_printf(
                result,
                "Number: %ld\nValid for: %02d.%02d.%04d\nTrip from: %02d.%02d.%04d %02d:%02d\nValidator: %05d",
                card_number,
                card_use_before_date_s.day,
                card_use_before_date_s.month,
                card_use_before_date_s.year,
                card_start_trip_minutes_s.day,
                card_start_trip_minutes_s.month,
                card_start_trip_minutes_s.year,
                card_start_trip_minutes_s.hour,
                card_start_trip_minutes_s.minute,
                card_validator);
            break;
        }
        case 0x1C2: {
            card_view = bit_lib_get_bits_16(data->block[32].value, 0, 10); //101
            card_type = bit_lib_get_bits_16(data->block[32].value, 10, 10); //102
            card_number = bit_lib_get_bits_32(data->block[32].value, 20, 32); //201
            card_layout = bit_lib_get_bits(data->block[32].value, 52, 4); //111
            card_layout2 = bit_lib_get_bits(data->block[32].value, 56, 5); //112
            uint16_t card_type_of_extended =
                bit_lib_get_bits_16(data->block[32].value, 61, 10); //122
            card_use_before_date = bit_lib_get_bits_16(data->block[32].value, 71, 16); //202.
            card_blank_type = bit_lib_get_bits_16(data->block[32].value, 87, 10); //121.
            uint16_t card_valid_to_minutes =
                bit_lib_get_bits_16(data->block[32].value, 97, 16); //311
            uint16_t card_activate_during =
                bit_lib_get_bits_16(data->block[32].value, 113, 9); //302
            uint32_t card_valid_for_minutes =
                bit_lib_get_bits_32(data->block[32].value, 131, 20); //314
            card_minutes_pass = bit_lib_get_bits(data->block[32].value, 154, 8); //412.
            uint8_t card_transport_type = bit_lib_get_bits(data->block[32].value, 163, 2); //421
            uint8_t card_passage_in_metro = bit_lib_get_bits(data->block[32].value, 165, 1); //431
            uint8_t card_transfer_in_metro = bit_lib_get_bits(data->block[32].value, 166, 1); //432
            uint16_t card_remaining_trips =
                bit_lib_get_bits_16(data->block[32].value, 167, 10); //321
            card_validator = bit_lib_get_bits_16(data->block[32].value, 177, 16); //422
            uint32_t card_start_trip_neg_minutes =
                bit_lib_get_bits_32(data->block[32].value, 196, 20); //404
            uint8_t card_requires_activation =
                bit_lib_get_bits(data->block[32].value, 216, 1); //301
            card_blocked = bit_lib_get_bits(data->block[32].value, 217, 1); //303
            uint8_t card_extended = bit_lib_get_bits(data->block[32].value, 218, 1); //123
            card_hash = bit_lib_get_bits_32(data->block[32].value, 224, 32); //502

            FURI_LOG_D(
                TAG,
                "%x %x %lx %x %x %x %x %x %x %x %lx %x %x %x %x %x %x %lx %x %x %x %lx",
                card_view,
                card_type,
                card_number,
                card_layout,
                card_layout2,
                card_type_of_extended,
                card_use_before_date,
                card_blank_type,
                card_valid_to_minutes,
                card_activate_during,
                card_valid_for_minutes,
                card_minutes_pass,
                card_transport_type,
                card_passage_in_metro,
                card_transfer_in_metro,
                card_remaining_trips,
                card_validator,
                card_start_trip_neg_minutes,
                card_requires_activation,
                card_blocked,
                card_extended,
                card_hash);
            FuriHalRtcDateTime card_use_before_date_s = {0};
            from_days_to_datetime(card_use_before_date, &card_use_before_date_s, 2016);

            FuriHalRtcDateTime card_start_trip_minutes_s = {0};
            from_minutes_to_datetime(
                card_start_trip_neg_minutes, &card_start_trip_minutes_s, 2016); //-time
            furi_string_printf(
                result,
                "Number: %ld\nValid for: %02d.%02d.%04d\nTrip from: %02d.%02d.%04d %02d:%02d\nValidator: %05d",
                card_number,
                card_use_before_date_s.day,
                card_use_before_date_s.month,
                card_use_before_date_s.year,
                card_start_trip_minutes_s.day,
                card_start_trip_minutes_s.month,
                card_start_trip_minutes_s.year,
                card_start_trip_minutes_s.hour,
                card_start_trip_minutes_s.minute,
                card_validator);
            break;
        }
        case 0x1C3: {
            card_view = bit_lib_get_bits_16(data->block[32].value, 0, 10); //101
            card_type = bit_lib_get_bits_16(data->block[32].value, 10, 10); //102
            card_number = bit_lib_get_bits_32(data->block[32].value, 20, 32); //201
            card_layout = bit_lib_get_bits(data->block[32].value, 52, 4); //111
            card_layout2 = bit_lib_get_bits(data->block[32].value, 56, 5); //112
            card_use_before_date = bit_lib_get_bits_16(data->block[32].value, 61, 16); //202
            card_blank_type = bit_lib_get_bits_16(data->block[32].value, 77, 10); //121
            card_remaining_funds = bit_lib_get_bits_32(data->block[32].value, 188, 22); //322
            card_hash = bit_lib_get_bits_32(data->block[32].value, 224, 32); //502
            card_validator = bit_lib_get_bits_16(data->block[32].value, 128, 16); //422
            card_start_trip_minutes = bit_lib_get_bits_32(data->block[32].value, 144, 23); //405
            uint8_t card_fare_trip = bit_lib_get_bits(data->block[32].value, 210, 2); //441
            card_minutes_pass = bit_lib_get_bits(data->block[32].value, 171, 7); //412
            uint8_t card_transport_type_flag =
                bit_lib_get_bits(data->block[32].value, 178, 2); //421.0
            uint8_t card_transport_type1 = bit_lib_get_bits(data->block[32].value, 180, 2); //421.1
            uint8_t card_transport_type2 = bit_lib_get_bits(data->block[32].value, 182, 2); //421.2
            uint8_t card_transport_type3 = bit_lib_get_bits(data->block[32].value, 184, 2); //421.3
            uint8_t card_transport_type4 = bit_lib_get_bits(data->block[32].value, 186, 2); //421.4
            card_blocked = bit_lib_get_bits(data->block[32].value, 212, 1); //303
            FURI_LOG_D(
                TAG,
                "%x %x %lx %x %x %x %x %lx %lx %x %lx %x %x %x %x %x %x %x %x",
                card_view,
                card_type,
                card_number,
                card_layout,
                card_layout2,
                card_use_before_date,
                card_blank_type,
                card_remaining_funds,
                card_hash,
                card_validator,
                card_start_trip_minutes,
                card_fare_trip,
                card_minutes_pass,
                card_transport_type_flag,
                card_transport_type1,
                card_transport_type2,
                card_transport_type3,
                card_transport_type4,
                card_blocked);
            FuriHalRtcDateTime card_use_before_date_s = {0};
            from_days_to_datetime(card_use_before_date, &card_use_before_date_s, 1992);

            FuriHalRtcDateTime card_start_trip_minutes_s = {0};
            from_minutes_to_datetime(card_start_trip_minutes, &card_start_trip_minutes_s, 2016);
            furi_string_printf(
                result,
                "Number: %010ld\nValid for: %02d.%02d.%04d\nBalance: %ld rub\nTrip from: %02d.%02d.%04d %02d:%02d\nValidator: %05d",
                card_number,
                card_use_before_date_s.day,
                card_use_before_date_s.month,
                card_use_before_date_s.year,
                card_remaining_funds / 100,
                card_start_trip_minutes_s.day,
                card_start_trip_minutes_s.month,
                card_start_trip_minutes_s.year,
                card_start_trip_minutes_s.hour,
                card_start_trip_minutes_s.minute,
                card_validator);
            break;
        }
        case 0x1C4: {
            card_view = bit_lib_get_bits_16(data->block[32].value, 0, 10); //101
            card_type = bit_lib_get_bits_16(data->block[32].value, 10, 10); //102
            card_number = bit_lib_get_bits_32(data->block[32].value, 20, 32); //201
            card_layout = bit_lib_get_bits(data->block[32].value, 52, 4); //111
            card_layout2 = bit_lib_get_bits(data->block[32].value, 56, 5); //112
            uint16_t card_type_of_extended =
                bit_lib_get_bits_16(data->block[32].value, 61, 10); //122
            card_use_before_date = bit_lib_get_bits_16(data->block[32].value, 71, 13); //202.
            card_blank_type = bit_lib_get_bits_16(data->block[32].value, 84, 10); //121.
            uint16_t card_valid_to_minutes =
                bit_lib_get_bits_16(data->block[32].value, 94, 13); //311
            uint16_t card_activate_during =
                bit_lib_get_bits_16(data->block[32].value, 107, 9); //302
            uint16_t card_extension_counter =
                bit_lib_get_bits_16(data->block[32].value, 116, 10); //304
            uint32_t card_valid_for_minutes =
                bit_lib_get_bits_32(data->block[32].value, 128, 20); //314
            card_minutes_pass = bit_lib_get_bits(data->block[32].value, 158, 7); //412.
            uint8_t card_transport_type_flag =
                bit_lib_get_bits(data->block[32].value, 178, 2); //421.0
            uint8_t card_transport_type1 = bit_lib_get_bits(data->block[32].value, 180, 2); //421.1
            uint8_t card_transport_type2 = bit_lib_get_bits(data->block[32].value, 182, 2); //421.2
            uint8_t card_transport_type3 = bit_lib_get_bits(data->block[32].value, 184, 2); //421.3
            uint8_t card_transport_type4 = bit_lib_get_bits(data->block[32].value, 186, 2); //421.4
            uint16_t card_remaining_trips =
                bit_lib_get_bits_16(data->block[32].value, 169, 10); //321
            card_validator = bit_lib_get_bits_16(data->block[32].value, 179, 16); //422
            uint32_t card_start_trip_neg_minutes =
                bit_lib_get_bits_32(data->block[32].value, 195, 20); //404
            uint8_t card_requires_activation =
                bit_lib_get_bits(data->block[32].value, 215, 1); //301
            card_blocked = bit_lib_get_bits(data->block[32].value, 216, 1); //303
            uint8_t card_extended = bit_lib_get_bits(data->block[32].value, 217, 1); //123
            card_hash = bit_lib_get_bits_32(data->block[32].value, 224, 32); //502

            FURI_LOG_D(
                TAG,
                "%x %x %lx %x %x %x %x %x %x %x %x %lx %x %x %x %x %x %x %x %x %lx %x %x %x %lx",
                card_view,
                card_type,
                card_number,
                card_layout,
                card_layout2,
                card_type_of_extended,
                card_use_before_date,
                card_blank_type,
                card_valid_to_minutes,
                card_activate_during,
                card_extension_counter,
                card_valid_for_minutes,
                card_minutes_pass,
                card_transport_type_flag,
                card_transport_type1,
                card_transport_type2,
                card_transport_type3,
                card_transport_type4,
                card_remaining_trips,
                card_validator,
                card_start_trip_neg_minutes,
                card_requires_activation,
                card_blocked,
                card_extended,
                card_hash);
            FuriHalRtcDateTime card_use_before_date_s = {0};
            from_days_to_datetime(card_use_before_date, &card_use_before_date_s, 2016);

            FuriHalRtcDateTime card_start_trip_minutes_s = {0};
            from_minutes_to_datetime(
                card_start_trip_neg_minutes, &card_start_trip_minutes_s, 2016); //-time
            furi_string_printf(
                result,
                "Number: %010ld\nValid for: %02d.%02d.%04d\nTrip from: %02d.%02d.%04d %02d:%02d\nValidator: %05d",
                card_number,
                card_use_before_date_s.day,
                card_use_before_date_s.month,
                card_use_before_date_s.year,
                card_start_trip_minutes_s.day,
                card_start_trip_minutes_s.month,
                card_start_trip_minutes_s.year,
                card_start_trip_minutes_s.hour,
                card_start_trip_minutes_s.minute,
                card_validator);
            break;
        }
        case 0x1C5: {
            card_view = bit_lib_get_bits_16(data->block[32].value, 0, 10); //101
            card_type = bit_lib_get_bits_16(data->block[32].value, 10, 10); //102
            card_number = bit_lib_get_bits_32(data->block[32].value, 20, 32); //201
            card_layout = bit_lib_get_bits(data->block[32].value, 52, 4); //111
            card_layout2 = bit_lib_get_bits(data->block[32].value, 56, 5); //112
            card_use_before_date = bit_lib_get_bits_16(data->block[32].value, 61, 13); //202.
            card_blank_type = bit_lib_get_bits_16(data->block[32].value, 74, 10); //121.
            uint32_t card_valid_to_time = bit_lib_get_bits_32(data->block[32].value, 84, 23); //317
            uint16_t card_extension_counter =
                bit_lib_get_bits_16(data->block[32].value, 107, 10); //304
            card_start_trip_minutes = bit_lib_get_bits_32(data->block[32].value, 128, 23); //405
            uint8_t card_metro_ride_with = bit_lib_get_bits(data->block[32].value, 151, 7); //414
            card_minutes_pass = bit_lib_get_bits(data->block[32].value, 158, 7); //412.
            card_remaining_funds = bit_lib_get_bits_32(data->block[32].value, 167, 19); //322
            card_validator = bit_lib_get_bits_16(data->block[32].value, 186, 16); //422
            card_blocked = bit_lib_get_bits(data->block[32].value, 202, 1); //303
            uint16_t card_route = bit_lib_get_bits_16(data->block[32].value, 204, 12); //424
            uint8_t card_passages_ground_transport =
                bit_lib_get_bits(data->block[32].value, 216, 7); //433
            card_hash = bit_lib_get_bits_32(data->block[32].value, 224, 32); //502

            FURI_LOG_D(
                TAG,
                "%x %x %lx %x %x %x %x %lx %x %lx %x %x %lx %x %x %x %x %lx",
                card_view,
                card_type,
                card_number,
                card_layout,
                card_layout2,
                card_use_before_date,
                card_blank_type,
                card_valid_to_time,
                card_extension_counter,
                card_start_trip_minutes,
                card_metro_ride_with,
                card_minutes_pass,
                card_remaining_funds,
                card_validator,
                card_blocked,
                card_route,
                card_passages_ground_transport,
                card_hash);
            FuriHalRtcDateTime card_use_before_date_s = {0};
            from_days_to_datetime(card_use_before_date, &card_use_before_date_s, 2019);

            FuriHalRtcDateTime card_start_trip_minutes_s = {0};
            from_minutes_to_datetime(card_start_trip_minutes, &card_start_trip_minutes_s, 2019);
            furi_string_printf(
                result,
                "Number: %010ld\nValid for: %02d.%02d.%04d\nBalance: %ld rub\nTrip from: %02d.%02d.%04d %02d:%02d\nValidator: %05d",
                card_number,
                card_use_before_date_s.day,
                card_use_before_date_s.month,
                card_use_before_date_s.year,
                card_remaining_funds / 100,
                card_start_trip_minutes_s.day,
                card_start_trip_minutes_s.month,
                card_start_trip_minutes_s.year,
                card_start_trip_minutes_s.hour,
                card_start_trip_minutes_s.minute,
                card_validator);
            break;
        }
        case 0x1C6: {
            card_view = bit_lib_get_bits_16(data->block[32].value, 0, 10); //101
            card_type = bit_lib_get_bits_16(data->block[32].value, 10, 10); //102
            card_number = bit_lib_get_bits_32(data->block[32].value, 20, 32); //201
            card_layout = bit_lib_get_bits(data->block[32].value, 52, 4); //111
            card_layout2 = bit_lib_get_bits(data->block[32].value, 56, 5); //112
            uint16_t card_type_of_extended =
                bit_lib_get_bits_16(data->block[32].value, 61, 10); //122
            card_use_before_date = bit_lib_get_bits_16(data->block[32].value, 71, 13); //202.
            card_blank_type = bit_lib_get_bits_16(data->block[32].value, 84, 10); //121.
            uint16_t card_valid_to_minutes =
                bit_lib_get_bits_16(data->block[32].value, 94, 23); //311
            uint16_t card_extension_counter =
                bit_lib_get_bits_16(data->block[32].value, 117, 10); //304
            uint32_t card_valid_for_minutes =
                bit_lib_get_bits_32(data->block[32].value, 128, 20); //314
            uint32_t card_start_trip_neg_minutes =
                bit_lib_get_bits_32(data->block[32].value, 148, 20); //404
            uint8_t card_metro_ride_with = bit_lib_get_bits(data->block[32].value, 168, 7); //414
            card_minutes_pass = bit_lib_get_bits(data->block[32].value, 175, 7); //412.
            uint16_t card_remaining_trips =
                bit_lib_get_bits_16(data->block[32].value, 169, 10); //321
            card_validator = bit_lib_get_bits_16(data->block[32].value, 189, 16); //422
            card_blocked = bit_lib_get_bits(data->block[32].value, 205, 1); //303
            uint8_t card_extended = bit_lib_get_bits(data->block[32].value, 206, 1); //123
            uint8_t card_route = bit_lib_get_bits(data->block[32].value, 212, 12); //424
            card_hash = bit_lib_get_bits_32(data->block[32].value, 224, 32); //502

            FURI_LOG_D(
                TAG,
                "%x %x %lx %x %x %x %x %x %x %x %lx %lx %x %x %x %x %x %x %x %lx",
                card_view,
                card_type,
                card_number,
                card_layout,
                card_layout2,
                card_type_of_extended,
                card_use_before_date,
                card_blank_type,
                card_valid_to_minutes,
                card_extension_counter,
                card_valid_for_minutes,
                card_start_trip_neg_minutes,
                card_metro_ride_with,
                card_minutes_pass,
                card_remaining_trips,
                card_validator,
                card_blocked,
                card_extended,
                card_route,
                card_hash);
            FuriHalRtcDateTime card_use_before_date_s = {0};
            from_days_to_datetime(card_use_before_date, &card_use_before_date_s, 2019);

            FuriHalRtcDateTime card_start_trip_minutes_s = {0};
            from_minutes_to_datetime(
                card_start_trip_neg_minutes, &card_start_trip_minutes_s, 2019); //-time
            furi_string_printf(
                result,
                "Number: %010ld\nValid for: %02d.%02d.%04d\nTrip from: %02d.%02d.%04d %02d:%02d\nValidator: %05d",
                card_number,
                card_use_before_date_s.day,
                card_use_before_date_s.month,
                card_use_before_date_s.year,
                card_start_trip_minutes_s.day,
                card_start_trip_minutes_s.month,
                card_start_trip_minutes_s.year,
                card_start_trip_minutes_s.hour,
                card_start_trip_minutes_s.minute,
                card_validator);
            break;
        }
        case 0x3CCB: {
            card_view = bit_lib_get_bits_16(data->block[32].value, 0, 10); //101
            card_type = bit_lib_get_bits_16(data->block[32].value, 10, 10); //102
            card_number = bit_lib_get_bits_32(data->block[32].value, 20, 32); //201
            card_layout = bit_lib_get_bits(data->block[32].value, 52, 4); //111
            uint16_t card_tech_code =
                bit_lib_get_bits_32(data->block[32].value, 56, 10); //tech_code
            uint16_t card_valid_to_minutes =
                bit_lib_get_bits_16(data->block[32].value, 66, 16); //311
            uint16_t card_valid_by_date = bit_lib_get_bits_16(data->block[32].value, 82, 16); //312
            uint8_t card_interval = bit_lib_get_bits(data->block[32].value, 98, 4); //interval
            uint16_t card_app_code1 =
                bit_lib_get_bits_16(data->block[32].value, 102, 16); //app_code1
            uint16_t card_hash1 = bit_lib_get_bits_16(data->block[32].value, 112, 16); //502.1
            uint16_t card_type1 = bit_lib_get_bits_16(data->block[32].value, 128, 10); //type1
            uint16_t card_app_code2 =
                bit_lib_get_bits_16(data->block[32].value, 138, 10); //app_code2
            uint16_t card_type2 = bit_lib_get_bits_16(data->block[32].value, 148, 10); //type2
            uint16_t card_app_code3 =
                bit_lib_get_bits_16(data->block[32].value, 158, 10); //app_code3
            uint16_t card_type3 = bit_lib_get_bits_16(data->block[32].value, 148, 10); //type3
            uint16_t card_app_code4 =
                bit_lib_get_bits_16(data->block[32].value, 168, 10); //app_code4
            uint16_t card_type4 = bit_lib_get_bits_16(data->block[32].value, 178, 10); //type4
            card_hash = bit_lib_get_bits_32(data->block[32].value, 224, 32); //502.2

            FURI_LOG_D(
                TAG,
                "%x %x %lx %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %lx",
                card_view,
                card_type,
                card_number,
                card_layout,
                card_tech_code,
                card_use_before_date,
                card_blank_type,
                card_valid_to_minutes,
                card_valid_by_date,
                card_interval,
                card_app_code1,
                card_hash1,
                card_type1,
                card_app_code2,
                card_type2,
                card_app_code3,
                card_type3,
                card_app_code4,
                card_type4,
                card_hash);
            FuriHalRtcDateTime card_use_before_date_s = {0};
            from_days_to_datetime(card_valid_by_date, &card_use_before_date_s, 1992);

            furi_string_printf(
                result,
                "Number: %010ld\nValid for: %02d.%02d.%04d\nValidator: %05d",
                card_number,
                card_use_before_date_s.day,
                card_use_before_date_s.month,
                card_use_before_date_s.year,
                card_validator);
            break;
        }
        case 0x3C0B: {
            card_view = bit_lib_get_bits_16(data->block[32].value, 0, 10); //101
            card_type = bit_lib_get_bits_16(data->block[32].value, 10, 10); //102
            card_number = bit_lib_get_bits_32(data->block[32].value, 20, 32); //201
            card_layout = bit_lib_get_bits(data->block[32].value, 52, 4); //111
            uint16_t card_tech_code =
                bit_lib_get_bits_32(data->block[32].value, 56, 10); //tech_code
            uint16_t card_valid_to_minutes =
                bit_lib_get_bits_16(data->block[32].value, 66, 16); //311
            uint16_t card_valid_by_date = bit_lib_get_bits_16(data->block[32].value, 82, 16); //312
            uint16_t card_hash = bit_lib_get_bits_16(data->block[32].value, 112, 16); //502.1

            FURI_LOG_D(
                TAG,
                "%x %x %lx %x %x %x %x %x %x %x",
                card_view,
                card_type,
                card_number,
                card_layout,
                card_tech_code,
                card_use_before_date,
                card_blank_type,
                card_valid_to_minutes,
                card_valid_by_date,
                card_hash);
            FuriHalRtcDateTime card_use_before_date_s = {0};
            from_days_to_datetime(card_valid_by_date, &card_use_before_date_s, 1992);

            furi_string_printf(
                result,
                "Number: %010ld\nValid for: %02d.%02d.%04d\nValidator: %05d",
                card_number,
                card_use_before_date_s.day,
                card_use_before_date_s.month,
                card_use_before_date_s.year,
                card_validator);
            break;
        }
        default:
            return false;
        }

        furi_string_printf(
            dev_data->parsed_data, "\e#Troika %x\n%s", layout_type, furi_string_get_cstr(result));
        troika_parsed = true;
    } while(false);

    return troika_parsed;
}
