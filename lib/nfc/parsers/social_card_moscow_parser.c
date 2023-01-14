#include "nfc_supported_card.h"

#include <gui/modules/widget.h>
#include <nfc_worker_i.h>
#include <nfc/helpers/transport.h>

static const MfClassicAuthContext social_card_moscow_keys[] = {
    {.sector = 0, .key_a = 0xa0a1a2a3a4a5, .key_b = 0x7de02a7f6025},
    {.sector = 1, .key_a = 0x2735fc181807, .key_b = 0xbf23a53c1f63},
    {.sector = 2, .key_a = 0x2aba9519f574, .key_b = 0xcb9a1f2d7368},
    {.sector = 3, .key_a = 0x84fd7f7a12b6, .key_b = 0xc7c0adb3284f},
    {.sector = 4, .key_a = 0x73068f118c13, .key_b = 0x2b7f3253fac5},
    {.sector = 5, .key_a = 0x186d8c4b93f9, .key_b = 0x9f131d8c2057},
    {.sector = 6, .key_a = 0x3a4bba8adaf0, .key_b = 0x67362d90f973},
    {.sector = 7, .key_a = 0x8765b17968a2, .key_b = 0x6202a38f69e2},
    {.sector = 8, .key_a = 0x40ead80721ce, .key_b = 0x100533b89331},
    {.sector = 9, .key_a = 0x0db5e6523f7c, .key_b = 0x653a87594079},
    {.sector = 10, .key_a = 0x51119dae5216, .key_b = 0xd8a274b2e026},
    {.sector = 11, .key_a = 0x51119dae5216, .key_b = 0xd8a274b2e026},
    {.sector = 12, .key_a = 0x51119dae5216, .key_b = 0xd8a274b2e026},
    {.sector = 13, .key_a = 0xa0a1a2a3a4a5, .key_b = 0x7de02a7f6025},
    {.sector = 14, .key_a = 0xa0a1a2a3a4a5, .key_b = 0x7de02a7f6025},
    {.sector = 15, .key_a = 0xa0a1a2a3a4a5, .key_b = 0x7de02a7f6025},
    {.sector = 16, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 17, .key_a = 0x2aba9519f574, .key_b = 0xcb9a1f2d7368},
    {.sector = 18, .key_a = 0x84fd7f7a12b6, .key_b = 0xc7c0adb3284f},
    {.sector = 19, .key_a = 0x2aba9519f574, .key_b = 0xcb9a1f2d7368},
    {.sector = 20, .key_a = 0x84fd7f7a12b6, .key_b = 0xc7c0adb3284f},
    {.sector = 21, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 22, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 23, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 24, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 25, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 26, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 27, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 28, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 29, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 30, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 31, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 32, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 33, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 34, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 35, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 36, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 37, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 38, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
    {.sector = 39, .key_a = 0xa229e68ad9e5, .key_b = 0x49c2b5296ef4},
};

bool social_card_moscow_parser_verify(NfcWorker* nfc_worker, FuriHalNfcTxRxContext* tx_rx) {
    furi_assert(nfc_worker);

    if(nfc_worker->dev_data->mf_classic_data.type != MfClassicType4k) {
        FURI_LOG_I("Social card moscow", "Card type mismatch");
        return false;
    }

    uint8_t sector = 15;
    uint8_t block = mf_classic_get_sector_trailer_block_num_by_sector(sector);
    FURI_LOG_D("Social card moscow", "Verifying sector %d", sector);
    if(mf_classic_authenticate(tx_rx, block, 0xa0a1a2a3a4a5, MfClassicKeyA)) {
        FURI_LOG_D("Social card moscow", "Sector %d verified", sector);
        return true;
    }
    FURI_LOG_I("Social card moscow", "Not verified");
    return false;
}

bool social_card_moscow_parser_read(NfcWorker* nfc_worker, FuriHalNfcTxRxContext* tx_rx) {
    furi_assert(nfc_worker);

    MfClassicReader reader = {};
    FuriHalNfcDevData* nfc_data = &nfc_worker->dev_data->nfc_data;
    reader.type = mf_classic_get_classic_type(nfc_data->atqa[0], nfc_data->atqa[1], nfc_data->sak);
    for(size_t i = 0; i < COUNT_OF(social_card_moscow_keys); i++) {
        mf_classic_reader_add_sector(
            &reader,
            social_card_moscow_keys[i].sector,
            social_card_moscow_keys[i].key_a,
            social_card_moscow_keys[i].key_b);
    }

    return mf_classic_read_card(tx_rx, &reader, &nfc_worker->dev_data->mf_classic_data) == 40;
}

bool social_card_moscow_parser_parse(NfcDeviceData* dev_data) {
    MfClassicData* data = &dev_data->mf_classic_data;

    // Verify key
    MfClassicSectorTrailer* sec_tr = mf_classic_get_sector_trailer_by_sector(data, 15);
    uint64_t key = nfc_util_bytes2num(sec_tr->key_a, 6);
    if(key != social_card_moscow_keys[15].key_a) return false;

    // Verify card type
    if(data->type != MfClassicType4k) return false;

    //Print social number

    uint8_t card_code_arr[3];
    uint32_t card_code = 0;
    uint8_t card_region = 0;
    uint8_t card_number_arr[5];
    uint64_t card_number = 0;
    uint8_t card_control = 0;
    uint8_t omc_number_arr[8];
    uint64_t omc_number = 0;
    uint8_t year = 0;
    uint8_t month = 0;

    for(uint8_t i = 0; i < 3; ++i) {
        card_code_arr[i] = data->block[60].value[i + 1];
    }

    for(uint8_t i = 0; i < 3; ++i) {
        card_code = (card_code << 8) | card_code_arr[i];
    }

    card_region = data->block[60].value[4];

    for(uint8_t i = 0; i < 5; ++i) {
        card_number_arr[i] = data->block[60].value[i + 5];
    }

    for(uint8_t i = 0; i < 5; ++i) {
        card_number = (card_number << 8) | card_number_arr[i];
    }

    card_control = data->block[60].value[10];
    card_control = card_control >> 4;

    for(uint8_t i = 0; i < 8; ++i) {
        omc_number_arr[i] = data->block[21].value[i + 1];
    }

    for(uint8_t i = 0; i < 8; ++i) {
        omc_number = (omc_number << 8) | omc_number_arr[i];
    }
    year = data->block[60].value[11];
    month = data->block[60].value[12];

    FuriString* result = furi_string_alloc();

    parse_transport_block(&data->block[4], result);

    furi_string_printf(
        dev_data->parsed_data,
        "\e#Social \ecard\nNumber: %lx %x %llx %x\nOMC:\n%llx\nValid for: %02x/%02x %02x%02x\n%s",
        card_code,
        card_region,
        card_number,
        card_control,
        omc_number,
        month,
        year,
        data->block[60].value[13],
        data->block[60].value[14],
        furi_string_get_cstr(result));

    return true;
}
