#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "crypto1.h"

#define MF_CLASSIC_MAX_DUMP_SIZE 1024

#define MAX_FRAME_SIZE 256 // maximum allowed ISO14443 frame
#define MAX_PARITY_SIZE ((MAX_FRAME_SIZE + 7) / 8)
#define MAX_MIFARE_FRAME_SIZE 18
#define MAX_MIFARE_PARITY_SIZE 3

#define MF_CLASSIC_REQ_CMD (0x26)
#define MF_CLASSIC_WAKEUP_CMD (0x52)
#define MF_CLASSIC_ANTICOLLISION_CL1_CMD (0x93, 0x20)
#define MF_CLASSIC_SELECT_CL1_CMD (0x93, 0x70)
#define MF_CLASSIC_ANTICOLLISION_CL2_CMD (0x95, 0x20)
#define MF_CLASSIC_SELECT_CL2_CMD (0x95, 0x70)
#define MF_CLASSIC_HALT_CMD (0x50, 0x00)
#define MF_CLASSIC_AUTH_A_CMD (0x60)
#define MF_CLASSIC_AUTH_B_CMD (0x61)
#define MF_CLASSIC_PERSONALIZE_CMD (0x40)
#define MF_CLASSIC_SET_MOD_TYPE_CMD (0x43)
#define MF_CLASSIC_READ_CMD (0x30)
#define MF_CLASSIC_WRITE_CMD (0xA0)
#define MF_CLASSIC_DECREMENT_CMD (0xC0)
#define MF_CLASSIC_INCREMENT_CMD (0xC1)
#define MF_CLASSIC_RESTORE_CMD (0xC2)
#define MF_CLASSIC_TRANSFER_CMD (0xB0)

#define ISO14443A_CMD_REQA 0x26
#define ISO14443A_CMD_READBLOCK 0x30
#define ISO14443A_CMD_WUPA 0x52
#define ISO14443A_CMD_OPTS 0x35
#define ISO14443A_CMD_ANTICOLL_OR_SELECT 0x93
#define ISO14443A_CMD_ANTICOLL_OR_SELECT_2 0x95
#define ISO14443A_CMD_ANTICOLL_OR_SELECT_3 0x97
#define ISO14443A_CMD_WRITEBLOCK 0xA0
#define ISO14443A_CMD_HALT 0x50
#define ISO14443A_CMD_RATS 0xE0
#define ISO14443A_CMD_PPS 0xD0
#define ISO14443A_CMD_NXP_DESELECT 0xC2

// Mifare 4k/2k/1k/mini Max Block / Max Sector
#define MIFARE_4K_MAXBLOCK 256
#define MIFARE_2K_MAXBLOCK 128
#define MIFARE_1K_MAXBLOCK 64
#define MIFARE_MINI_MAXBLOCK 20

#define MIFARE_MINI_MAXSECTOR 5
#define MIFARE_1K_MAXSECTOR 16
#define MIFARE_2K_MAXSECTOR 32
#define MIFARE_4K_MAXSECTOR 40

// mifare authentication
#define CRYPT_NONE 0
#define CRYPT_ALL 1
#define CRYPT_REQUEST 2
#define AUTH_FIRST 0
#define AUTH_NESTED 2

typedef enum {
    MfClassicTypeUnknown,
    MfClassicTypeS50, // 1k
    MfClassicTypeS70, // 4k
} MfClassicType;

typedef struct {
    uint8_t header;
    uint8_t vendor_id;
    uint8_t prod_type;
    uint8_t prod_subtype;
    uint8_t prod_ver_major;
    uint8_t prod_ver_minor;
    uint8_t storage_size;
    uint8_t protocol_type;
} MfClassicVersion;

typedef struct {
    MfClassicVersion version;
    uint8_t signature[32];
    uint16_t counter[3];
    uint8_t tearing[3];
    uint16_t data_size;
    uint8_t data[MF_CLASSIC_MAX_DUMP_SIZE];
} MifareClassicData;

typedef struct {
    MfClassicType type;
    uint8_t blocks_to_read;
    uint8_t blocks_read;
    bool support_fast_read;
    bool data_changed;
    MifareClassicData data;
} MifareClassicDevice;

bool mf_classic_check_card_type(uint8_t ATQA0, uint8_t ATQA1, uint8_t SAK);

void mf_classic_set_default_version(MifareClassicDevice* mf_classic_read);

uint16_t mf_classic_read_block(struct Crypto1State* pcs, uint32_t uid, uint8_t blockNo, uint8_t *blockData);

void mf_classic_parse_read_response(uint8_t* buff, uint16_t block_addr, MifareClassicDevice* mf_classic_read);

void MifareReadBlock(uint8_t blockNo, uint8_t keyType, uint64_t ui64Key, uint8_t uid);

int mifare_classic_auth(
    struct Crypto1State* pcs,
    uint32_t uid,
    uint8_t blockNo,
    uint8_t keyType,
    uint64_t ui64Key,
    uint8_t isNested);
int mifare_classic_authex(
    struct Crypto1State* pcs,
    uint32_t uid,
    uint8_t blockNo,
    uint8_t keyType,
    uint64_t ui64Key,
    uint8_t isNested,
    uint32_t* ntptr,
    uint32_t* timing);

int mifare_classic_halt(struct Crypto1State* pcs);