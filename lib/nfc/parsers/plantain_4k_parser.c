#include "nfc_supported_card.h"
#include "plantain_parser.h" // For luhn

#include <gui/modules/widget.h>
#include <nfc_worker_i.h>

#include "furi_hal.h"

static const MfClassicAuthContext plantain_keys_4k[] = {
    {.sector = 0, .key_a = 0xFFFFFFFFFFFF, .key_b = 0xFFFFFFFFFFFF},
    {.sector = 1, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 2, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 3, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 4, .key_a = 0xe56ac127dd45, .key_b = 0x19fc84a3784b},
    {.sector = 5, .key_a = 0x77dabc9825e1, .key_b = 0x9764fec3154a},
    {.sector = 6, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 7, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 8, .key_a = 0x26973ea74321, .key_b = 0xd27058c6e2c7},
    {.sector = 9, .key_a = 0xeb0a8ff88ade, .key_b = 0x578a9ada41e3},
    {.sector = 10, .key_a = 0xea0fd73cb149, .key_b = 0x29c35fa068fb},
    {.sector = 11, .key_a = 0xc76bf71a2509, .key_b = 0x9ba241db3f56},
    {.sector = 12, .key_a = 0xacffffffffff, .key_b = 0x71f3a315ad26},
    {.sector = 13, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 14, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 15, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 16, .key_a = 0x72f96bdd3714, .key_b = 0x462225cd34cf},
    {.sector = 17, .key_a = 0x044ce1872bc3, .key_b = 0x8c90c70cff4a},
    {.sector = 18, .key_a = 0xbc2d1791dec1, .key_b = 0xca96a487de0b},
    {.sector = 19, .key_a = 0x8791b2ccb5c4, .key_b = 0xc956c3b80da3},
    {.sector = 20, .key_a = 0x8e26e45e7d65, .key_b = 0x8e65b3af7d22},
    {.sector = 21, .key_a = 0x0f318130ed18, .key_b = 0x0c420a20e056},
    {.sector = 22, .key_a = 0x045ceca15535, .key_b = 0x31bec3d9e510},
    {.sector = 23, .key_a = 0x9d993c5d4ef4, .key_b = 0x86120e488abf},
    {.sector = 24, .key_a = 0xc65d4eaa645b, .key_b = 0xb69d40d1a439},
    {.sector = 25, .key_a = 0x3a8a139c20b4, .key_b = 0x8818a9c5d406},
    {.sector = 26, .key_a = 0xbaff3053b496, .key_b = 0x4b7cb25354d3},
    {.sector = 27, .key_a = 0x7413b599c4ea, .key_b = 0xb0a2AAF3A1BA},
    {.sector = 28, .key_a = 0x0ce7cd2cc72b, .key_b = 0xfa1fbb3f0f1f},
    {.sector = 29, .key_a = 0x0be5fac8b06a, .key_b = 0x6f95887a4fd3},
    {.sector = 30, .key_a = 0x0eb23cc8110b, .key_b = 0x04dc35277635},
    {.sector = 31, .key_a = 0xbc4580b7f20b, .key_b = 0xd0a4131fb290},
    {.sector = 32, .key_a = 0x7a396f0d633d, .key_b = 0xad2bdc097023},
    {.sector = 33, .key_a = 0xa3faa6daff67, .key_b = 0x7600e889adf9},
    {.sector = 34, .key_a = 0xfd8705e721b0, .key_b = 0x296fc317a513},
    {.sector = 35, .key_a = 0x22052b480d11, .key_b = 0xe19504c39461},
    {.sector = 36, .key_a = 0xa7141147d430, .key_b = 0xff16014fefc7},
    {.sector = 37, .key_a = 0x8a8d88151a00, .key_b = 0x038b5f9b5a2a},
    {.sector = 38, .key_a = 0xb27addfb64b0, .key_b = 0x152fd0c420a7},
    {.sector = 39, .key_a = 0x7259fa0197c6, .key_b = 0x5583698df085},
};

static const MfClassicAuthContext SPB_type_1[] = {
    {.sector = 4, .key_a = 0xe56ac127dd45, .key_b = 0x19fc84a3784b},
    {.sector = 5, .key_a = 0x77dabc9825e1, .key_b = 0x9764fec3154a},
    {.sector = 8, .key_a = 0x26973ea74321, .key_b = 0xd27058c6e2c7},
    {.sector = 12, .key_a = 0xacffffffffff, .key_b = 0x71f3a315ad26},
};

static const MfClassicAuthContext SPB_type_2[] = {
    {.sector = 4, .key_a = 0xe56ac127dd45, .key_b = 0x19fc84a3784b},
    {.sector = 5, .key_a = 0x77dabc9825e1, .key_b = 0x9764fec3154a},
    {.sector = 8, .key_a = 0xa73f5dc1d333, .key_b = 0xd27058c6e2c7}, //B unknown, TODO
};

static const MfClassicAuthContext SPB_type_3[] = {
    {.sector = 8, .key_a = 0x26973ea74321, .key_b = 0xd27058c6e2c7},
    {.sector = 9, .key_a = 0xeb0a8ff88ade, .key_b = 0x578a9ada41e3},
    {.sector = 12, .key_a = 0x000000000000, .key_b = 0x71f3a315ad26},
    {.sector = 13, .key_a = 0xac70ca327a04, .key_b = 0xf29411c2663c},
    {.sector = 14, .key_a = 0x51044efb5aab, .key_b = 0xebdc720dd1ce},
};

bool check_card_type(FuriHalNfcTxRxContext* tx_rx, uint8_t type) {
    // furi_assert(nfc_worker);
    // UNUSED(nfc_worker);

    const MfClassicAuthContext* auth_context;
    uint8_t auth_context_size;

    if(type == 1) {
        auth_context = SPB_type_1;
        auth_context_size = COUNT_OF(SPB_type_1);
    } else if(type == 2) {
        auth_context = SPB_type_2;
        auth_context_size = COUNT_OF(SPB_type_2);
    } else if(type == 3) {
        auth_context = SPB_type_3;
        auth_context_size = COUNT_OF(SPB_type_3);
    } else {
        return false;
    }
    uint8_t counter = 0;
    for(size_t i = 0; i < auth_context_size; i++) {
        // check if we can read sector
        if(mf_classic_authenticate(
               tx_rx,
               mf_classic_get_sector_trailer_block_num_by_sector(auth_context[i].sector),
               auth_context[i].key_a,
               MfClassicKeyA)) {
            counter++;
        } else {
            FURI_LOG_D(
                "Plant4k",
                "Error: Type: %d, Sector: %d, Key A: %x",
                type,
                auth_context[i].sector,
                auth_context[i].key_a);
        }
    }
    if(counter == auth_context_size) {
        return true;
    }

    return false;
}

uint8_t get_card_type(FuriHalNfcTxRxContext* tx_rx) {
    uint8_t card_type = 0;

    if(check_card_type(tx_rx, 1)) {
        card_type = 1;
    } else if(check_card_type(tx_rx, 2)) {
        card_type = 2;
    } else if(check_card_type(tx_rx, 3)) {
        card_type = 3;
    }
    return card_type;
}

bool plantain_4k_parser_verify(NfcWorker* nfc_worker, FuriHalNfcTxRxContext* tx_rx) {
    furi_assert(nfc_worker);
    UNUSED(nfc_worker);

    if(nfc_worker->dev_data->mf_classic_data.type != MfClassicType4k) {
        return false;
    }

    //     uint8_t sector = 8;
    //     uint8_t block = mf_classic_get_sector_trailer_block_num_by_sector(sector);
    //     FURI_LOG_D("Plant4K", "Verifying sector %d", sector);
    //     if(mf_classic_authenticate(tx_rx, block, 0x26973ea74321, MfClassicKeyA)) {
    //         FURI_LOG_D("Plant4K", "Sector %d verified", sector);
    //         return true;
    //     }
    //     return false;

    uint8_t card_type = get_card_type(tx_rx);
    if(card_type) {
        FURI_LOG_D("Plant4K", "SPB type %d", card_type);
        return true;
    }

    return false;
}

static uint8_t mf_classic_get_blocks_num_in_sector(uint8_t sector) {
    furi_assert(sector < 40);
    return sector < 32 ? 4 : 16;
}

static uint8_t mf_classic_get_first_block_num_of_sector(uint8_t sector) {
    furi_assert(sector < 40);
    if(sector < 32) {
        return sector * 4;
    } else {
        return 32 * 4 + (sector - 32) * 16;
    }
}

void add_fake_sector(
    uint8_t sector_num,
    MfClassicData* data,
    uint64_t key_a,
    uint64_t key_b,
    MfClassicSector* temp_sector) {
    uint8_t first_block = mf_classic_get_first_block_num_of_sector(sector_num);
    for(uint8_t j = 0; j < temp_sector->total_blocks; j++) {
        mf_classic_set_block_read(data, first_block + j, &(temp_sector->block[j]));
    }
    if(key_a != MF_CLASSIC_NO_KEY) {
        mf_classic_set_key_found(data, sector_num, MfClassicKeyA, key_a);
    }
    if(key_b != MF_CLASSIC_NO_KEY) {
        mf_classic_set_key_found(data, sector_num, MfClassicKeyB, key_b);
    }
}

bool plantain_4k_parser_read(NfcWorker* nfc_worker, FuriHalNfcTxRxContext* tx_rx) {
    furi_assert(nfc_worker);

    uint8_t counter = 0;
    FuriHalNfcDevData* nfc_data = &nfc_worker->dev_data->nfc_data;
    MfClassicReader reader = {};

    reader.type = mf_classic_get_classic_type(nfc_data->atqa[0], nfc_data->atqa[1], nfc_data->sak);
    MfClassicReader reader_copy = reader;
    uint8_t card_type = get_card_type(tx_rx);
    if(!card_type) {
        return false;
    }

    FURI_LOG_D("Plant4K", "Card type: %d", card_type);

    switch(card_type) {
    case 1:

        for(size_t i = 0; i < COUNT_OF(plantain_keys_4k); i++) {
            mf_classic_reader_add_sector(
                &reader,
                plantain_keys_4k[i].sector,
                plantain_keys_4k[i].key_a,
                plantain_keys_4k[i].key_b);
            FURI_LOG_T("plant4k", "Added sector %d", plantain_keys_4k[i].sector);
        }
        for(int i = 0; i < 5; i++) {
            if(mf_classic_read_card(tx_rx, &reader, &nfc_worker->dev_data->mf_classic_data) ==
               40) {
                return true;
            }
        }
        break;
    case 2:
        counter = 0;
        reader = reader_copy;
        for(size_t i = 0; i < COUNT_OF(plantain_keys_4k); i++) {
            bool readed = false;
            mf_classic_reader_add_sector(
                &reader,
                plantain_keys_4k[i].sector,
                plantain_keys_4k[i].key_a,
                plantain_keys_4k[i].key_b);
            for(int i = 0; i < 5; i++) {
                if(mf_classic_read_card(tx_rx, &reader, &nfc_worker->dev_data->mf_classic_data) ==
                   counter + 1) {
                    readed = true;
                    break;
                }
            }
            if(readed) {
                counter++;
            } else {
                if(plantain_keys_4k[i].sector == 1) {
                    // Manually add sector 1
                    MfClassicSector temp_sector = {};
                    temp_sector.total_blocks =
                        mf_classic_get_blocks_num_in_sector(plantain_keys_4k[i].sector);
                    memset(temp_sector.block, 0, sizeof(temp_sector.block));
                    add_fake_sector(
                        plantain_keys_4k[i].sector,
                        &nfc_worker->dev_data->mf_classic_data,
                        plantain_keys_4k[i].key_a,
                        plantain_keys_4k[i].key_b,
                        &temp_sector);
                    counter++;
                }
            }

            // FURI_LOG_T("plant4k", "Added sector %d", plantain_keys_4k[i].sector);
        }
        if(counter == 40) {
            return true;
        }

        break;
    case 3:
        counter = 0;
        reader = reader_copy;
        for(size_t i = 0; i < COUNT_OF(plantain_keys_4k); i++) {
            bool readed = false;
            uint8_t sector_num = plantain_keys_4k[i].sector;
            uint8_t variants_count = 0;
            MfClassicAuthContext sector_auth_variants[40] = {plantain_keys_4k[i]};
            for(uint8_t j = 0; j < COUNT_OF(SPB_type_3) && variants_count < 41; j++) {
                if(SPB_type_3[j].sector == sector_num) {
                    sector_auth_variants[++variants_count] = SPB_type_3[j];
                    break;
                }
            }

            for(uint8_t j = 0; j < variants_count; j++) {
                if(mf_classic_authenticate(
                       tx_rx,
                       mf_classic_get_sector_trailer_block_num_by_sector(
                           sector_auth_variants[j].sector),
                       sector_auth_variants[j].key_a,
                       MfClassicKeyA) &&
                   mf_classic_authenticate(
                       tx_rx,
                       mf_classic_get_sector_trailer_block_num_by_sector(
                           sector_auth_variants[j].sector),
                       sector_auth_variants[j].key_b,
                       MfClassicKeyB)) {
                    mf_classic_reader_add_sector(
                        &reader,
                        sector_auth_variants[j].sector,
                        sector_auth_variants[j].key_a,
                        sector_auth_variants[j].key_b);
                    for(int l = 0; l < 5; l++) {
                        if(mf_classic_read_card(
                               tx_rx, &reader, &nfc_worker->dev_data->mf_classic_data) ==
                           counter + 1) {
                            readed = true;
                            break;
                        }
                    }
                    if (readed) {
                        break;
                    }
                    break;
                }
                if (readed) {
                    counter++;
                    break;
                }
                else{
                    
                }
            }

            mf_classic_reader_add_sector(
                &reader,
                plantain_keys_4k[i].sector,
                plantain_keys_4k[i].key_a,
                plantain_keys_4k[i].key_b);
            for(int i = 0; i < 5; i++) {
                if(mf_classic_read_card(tx_rx, &reader, &nfc_worker->dev_data->mf_classic_data) ==
                   counter + 1) {
                    readed = true;
                    break;
                }
            }
            if(readed) {
                counter++;
            } else {
                if(plantain_keys_4k[i].sector == 1) {
                    // Manually add sector 1
                    MfClassicSector temp_sector = {};
                    temp_sector.total_blocks =
                        mf_classic_get_blocks_num_in_sector(plantain_keys_4k[i].sector);
                    memset(temp_sector.block, 0, sizeof(temp_sector.block));
                    add_fake_sector(
                        plantain_keys_4k[i].sector,
                        &nfc_worker->dev_data->mf_classic_data,
                        plantain_keys_4k[i].key_a,
                        plantain_keys_4k[i].key_b,
                        &temp_sector);
                    counter++;
                }
            }

            // FURI_LOG_T("plant4k", "Added sector %d", plantain_keys_4k[i].sector);
        }
        if(counter == 40) {
            return true;
        }
        break;
    }

    return false;
}

bool plantain_4k_parser_parse(NfcDeviceData* dev_data) {
    MfClassicData* data = &dev_data->mf_classic_data;

    // Verify key
    // if(key != plantain_keys_4k[8].key_a) return false;

    // get card type
    uint8_t card_type = 0;
    FURI_LOG_D("plant4k", "%d, %d", nfc_util_bytes2num(mf_classic_get_sector_trailer_by_sector(data, 8)->key_a, 6), SPB_type_1[3].key_a);
    if (nfc_util_bytes2num(mf_classic_get_sector_trailer_by_sector(data, 8)->key_a, 6) == SPB_type_1[3].key_a){
        FURI_LOG_D("plant4k", "Key A, sec 8,is SPB_type_1 or 3");
        if (nfc_util_bytes2num(mf_classic_get_sector_trailer_by_sector(data, 12)->key_a, 6) == plantain_keys_4k[12].key_a){
            FURI_LOG_D("plant4k", "Key A, sec 12, is SPB_type_1");
            card_type = 1;
        }
        else if (nfc_util_bytes2num(mf_classic_get_sector_trailer_by_sector(data, 12)->key_a, 6) == SPB_type_3[3].key_a){
            FURI_LOG_D("plant4k", "Key A, sec 12, is SPB_type_3");
            card_type = 3;
        }

    }
    else if (nfc_util_bytes2num(mf_classic_get_sector_trailer_by_sector(data, 8)->key_a, 6) == SPB_type_2[0].key_a){
        FURI_LOG_D("plant4k", "Key A, sec 8,is SPB_type_2");
        card_type = 2;
    }

    if (!card_type) return false;

    // Point to block 0 of sector 4, value 0
    uint8_t* temp_ptr = &data->block[4 * 4].value[0];
    // Read first 4 bytes of block 0 of sector 4 from last to first and convert them to uint32_t
    // 38 18 00 00 becomes 00 00 18 38, and equals to 6200 decimal
    uint32_t balance =
        ((temp_ptr[3] << 24) | (temp_ptr[2] << 16) | (temp_ptr[1] << 8) | temp_ptr[0]) / 100;
    // Read card number
    // Point to block 0 of sector 0, value 0
    temp_ptr = &data->block[0 * 4].value[0];
    // Read first 7 bytes of block 0 of sector 0 from last to first and convert them to uint64_t
    // 80 5C 23 8A 16 31 04 becomes 04 31 16 8A 23 5C 80, and equals to 36130104729284868 decimal
    uint8_t card_number_arr[7];
    for(size_t i = 0; i < 7; i++) {
        card_number_arr[i] = temp_ptr[6 - i];
    }
    // Copy card number to uint64_t
    uint64_t card_number = 0;
    for(size_t i = 0; i < 7; i++) {
        card_number = (card_number << 8) | card_number_arr[i];
    }
    // Convert card number to string
    FuriString* card_number_str;
    card_number_str = furi_string_alloc();
    // Should look like "361301047292848684"
    furi_string_printf(card_number_str, "%llu", card_number);
    // Add suffix with luhn checksum (1 digit) to the card number string
    FuriString* card_number_suffix;
    card_number_suffix = furi_string_alloc();

    furi_string_cat_printf(card_number_suffix, "-");
    furi_string_cat_printf(card_number_str, furi_string_get_cstr(card_number_suffix));
    // Free all not needed strings
    furi_string_free(card_number_suffix);

    furi_string_printf(
        dev_data->parsed_data,
        "\e#Plantain\nN:%s\nBalance:%d\nCard type:%d",
        furi_string_get_cstr(card_number_str),
        balance, card_type);
    furi_string_free(card_number_str);

    return true;
}
