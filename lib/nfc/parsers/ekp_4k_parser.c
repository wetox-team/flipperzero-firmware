#include "nfc_supported_card.h"
#include "plantain_parser.h" // For luhn and string_push_uint64

#include <gui/modules/widget.h>
#include <nfc_worker_i.h>

#include "furi_hal.h"

static const MfClassicAuthContext ekp_keys_4k[] = {
    {.sector = 0, .key_a = 0xe99d324cf585, .key_b = 0x851B65437989},
    {.sector = 1, .key_a = 0x2EAE5C516740, .key_b = 0x6609613755A0},
    {.sector = 2, .key_a = 0x649147098F20, .key_b = 0xC48839DC38E2},
    {.sector = 3, .key_a = 0x53782EC66D01, .key_b = 0x108ACC343299},
    {.sector = 4, .key_a = 0xECC4298EEC30, .key_b = 0x120835B3D8E8},
    {.sector = 5, .key_a = 0x6E79CB8F0FED, .key_b = 0xDC0CE86D91A3},
    {.sector = 6, .key_a = 0x90A7A5B9D295, .key_b = 0xF3AF7CD3BA28},
    {.sector = 7, .key_a = 0xED42FE935A1F, .key_b = 0x5658FFFB83D3},
    {.sector = 8, .key_a = 0x473279B02C01, .key_b = 0x46F3D7B3A9DE},
    {.sector = 9, .key_a = 0x758698638CA3, .key_b = 0x0EE37DCE85DD},
    {.sector = 10, .key_a = 0xF414407E3EEB, .key_b = 0x1DD699791925},
    {.sector = 11, .key_a = 0x77F7A84A4796, .key_b = 0x38DF598A01B1},
    {.sector = 12, .key_a = 0x01B1710A6659, .key_b = 0xA05D0101797F},
    {.sector = 13, .key_a = 0x4BE6BAA66014, .key_b = 0x59A10446E65F},
    {.sector = 14, .key_a = 0x8BF16F5C6A78, .key_b = 0x066038C789A6},
    {.sector = 15, .key_a = 0xE7C4339331FA, .key_b = 0x0054E9124C5A},
    {.sector = 16, .key_a = 0x18E3A02B5EFF, .key_b = 0x96A6D98CCEB0},
    {.sector = 17, .key_a = 0x1B61B2E78C75, .key_b = 0x9DFC804D2A00},
    {.sector = 18, .key_a = 0xE328A1C7156D, .key_b = 0xC34C0BDE0C83},
    {.sector = 19, .key_a = 0x6B07877E2C5C, .key_b = 0x54C7F1CE1C78},
    {.sector = 20, .key_a = 0xB33680481F0C, .key_b = 0x4B842862D496},
    {.sector = 21, .key_a = 0x666239D6C36A, .key_b = 0xF3B741D939D8},
    {.sector = 22, .key_a = 0x47650B145DCA, .key_b = 0x70A1FAF82208},
    {.sector = 23, .key_a = 0xA0381CF99334, .key_b = 0xE98D18EBF6A7},
    {.sector = 24, .key_a = 0x4FD02BB025E0, .key_b = 0x3B7084A60C03},
    {.sector = 25, .key_a = 0x30EDF4B996FC, .key_b = 0x5915104D1EA6},
    {.sector = 26, .key_a = 0xF50457E5ED4B, .key_b = 0xD5E5585A7A6C},
    {.sector = 27, .key_a = 0xBFDEAF3C3A52, .key_b = 0x33B2997ED7E9},
    {.sector = 28, .key_a = 0xF5800A359311, .key_b = 0xEC168579FDEA},
    {.sector = 29, .key_a = 0x318724D6A882, .key_b = 0x63486669B546},
    {.sector = 30, .key_a = 0x4A04579C2A5B, .key_b = 0x23870E40A13A},
    {.sector = 31, .key_a = 0xCD7BEE399FD2, .key_b = 0x126B746EE60F},
    {.sector = 32, .key_a = 0x7A396F0D633D, .key_b = 0xAD2BDC097023},
    {.sector = 33, .key_a = 0xA3FAA6DAFF67, .key_b = 0x7600E889ADF9},
    {.sector = 34, .key_a = 0xFD8705E721B0, .key_b = 0x296FC317A513},
    {.sector = 35, .key_a = 0xC9822A101508, .key_b = 0x88819B6B2632},
    {.sector = 36, .key_a = 0x424DA92F838A, .key_b = 0xFDDFF8FE692E},
    {.sector = 37, .key_a = 0x6141FE928401, .key_b = 0xC744FF0BAA94},
    {.sector = 38, .key_a = 0x1EDF969F11D0, .key_b = 0x34DB430A0498},
    {.sector = 39, .key_a = 0xE341067BFB71, .key_b = 0x61C928D7231B},
};

bool ekp_4k_parser_verify(NfcWorker* nfc_worker, FuriHalNfcTxRxContext* tx_rx) {
    furi_assert(nfc_worker);
    UNUSED(nfc_worker);

    if(nfc_worker->dev_data->mf_classic_data.type != MfClassicType4k) {
        return false;
    }

    uint8_t sector = 31;
    uint8_t block = mf_classic_get_sector_trailer_block_num_by_sector(sector);
    FURI_LOG_D("conc4k", "Verifying sector %d", sector);
    if(mf_classic_authenticate(tx_rx, block, 0xCD7BEE399FD2, MfClassicKeyA)) {
        FURI_LOG_D("conc4k", "Sector %d verified", sector);
        return true;
    }
    return false;
}

bool ekp_4k_parser_read(NfcWorker* nfc_worker, FuriHalNfcTxRxContext* tx_rx) {
    furi_assert(nfc_worker);

    MfClassicReader reader = {};
    FuriHalNfcDevData* nfc_data = &nfc_worker->dev_data->nfc_data;
    reader.type = mf_classic_get_classic_type(nfc_data->atqa[0], nfc_data->atqa[1], nfc_data->sak);
    // size_t keys_num = 3; //TODO: calc keys_num
    // for (size_t keys_index = 0; keys_index < keys_num; keys_index++) {
    for(size_t i = 0; i < COUNT_OF(ekp_keys_4k); i++) {
        // FURI_LOG_D("conc4k", "Iter %d", keys_index);
        // for(size_t i = 0; i < ; i++) {
        mf_classic_reader_add_sector(
            &reader,
            ekp_keys_4k[i].sector,
            ekp_keys_4k[i].key_a,
            ekp_keys_4k[i].key_b);
        FURI_LOG_T("conc4k", "Added sector %d", ekp_keys_4k[i].sector);
    }
    for(int i = 0; i < 5; i++) {
        uint8_t sectors_readed =
            mf_classic_read_card(tx_rx, &reader, &nfc_worker->dev_data->mf_classic_data);
        FURI_LOG_D("conc4k", "sectors_readed %d", sectors_readed);
        if(sectors_readed == 40) {
            return true;
        }
        return false;
    }
    // }
    return false;
}

bool ekp_4k_parser_parse(NfcDeviceData* dev_data) {
    MfClassicData* data = &dev_data->mf_classic_data;

    // Verify key
    MfClassicSectorTrailer* sec_tr = mf_classic_get_sector_trailer_by_sector(data, 8);
    uint64_t key = nfc_util_bytes2num(sec_tr->key_a, 6);
    if(key != ekp_keys_4k[8].key_a) return false;

    // Point to block 0 of sector 4, value 0
    // uint8_t* temp_ptr = &data->block[4 * 4].value[0];
    // Read first 4 bytes of block 0 of sector 4 from last to first and convert them to uint32_t
    // 38 18 00 00 becomes 00 00 18 38, and equals to 6200 decimal
    // uint32_t balance =
    //     ((temp_ptr[3] << 24) | (temp_ptr[2] << 16) | (temp_ptr[1] << 8) | temp_ptr[0]) / 100;

    // Read EKP num from block 0 from sector 32, bytes 1-8, big endian
    
    // Point to block 0 of sector 32
    uint8_t* temp_ptr = &data->block[32 * 4].value[0];
    // Read bytes 1-8 of block 0 of sector 32
    uint64_t ekp_num = (uint64_t)(*(uint64_t*)&temp_ptr[0]);
    // Convert to little endian
    ekp_num = __builtin_bswap64(ekp_num);


    string_printf(
        dev_data->parsed_data,
        "\e#Pure EKP\nNum:%llu\n",
        ekp_num);
    return true;
}
