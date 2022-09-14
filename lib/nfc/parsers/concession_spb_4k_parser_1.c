#include "nfc_supported_card.h"
#include "plantain_parser.h" // For luhn and string_push_uint64

#include <gui/modules/widget.h>
#include <nfc_worker_i.h>

#include "furi_hal.h"

// static const MfClassicAuthContext concession_spb_keys_4k[] = {
//     {.sector = 0, .key_a = 0xa0a1a2a3a4a5, .key_b = 0xb0b1b2b3b4b5},
//     {.sector = 1, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
//     {.sector = 2, .key_a = 0xa0a1a2a3a4a5, .key_b = 0xb0b1b2b3b4b5},
//     {.sector = 3, .key_a = 0xa0a1a2a3a4a5, .key_b = 0xb0b1b2b3b4b5},
//     {.sector = 4, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
//     {.sector = 5, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
//     {.sector = 6, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
//     {.sector = 7, .key_a = 0x2066f4727129, .key_b = 0xf7a65799c6ee},
//     {.sector = 8, .key_a = 0x26973ea74321, .key_b = 0xd27058c6e2c7},
//     {.sector = 9, .key_a = 0xeb0a8ff88ade, .key_b = 0x578a9ada41e3},
//     {.sector = 10, .key_a = 0xea0fd73cb149, .key_b = 0x29c35fa068fb},
//     {.sector = 11, .key_a = 0xc76bf71a2509, .key_b = 0x9ba241db3f56},
//     {.sector = 12, .key_a = 0x000000000000, .key_b = 0x71f3a315ad26},
//     {.sector = 13, .key_a = 0xac70ca327a04, .key_b = 0xf29411c2663c},
//     {.sector = 14, .key_a = 0x51044efb5aab, .key_b = 0xebdc720dd1ce},
//     {.sector = 15, .key_a = 0xa0a1a2a3a4a5, .key_b = 0x103c08acceb2},
//     {.sector = 16, .key_a = 0x72f96bdd3714, .key_b = 0x462225cd34cf},
//     {.sector = 17, .key_a = 0x044ce1872bc3, .key_b = 0x8c90c70cff4a},
//     {.sector = 18, .key_a = 0xbc2d1791dec1, .key_b = 0xca96a487de0b},
//     {.sector = 19, .key_a = 0x8791b2ccb5c4, .key_b = 0xc956c3b80da3},
//     {.sector = 20, .key_a = 0x8e26e45e7d65, .key_b = 0x8e65b3af7d22},
//     {.sector = 21, .key_a = 0x0f318130ed18, .key_b = 0x0c420a20e056},
//     {.sector = 22, .key_a = 0x045ceca15535, .key_b = 0x31bec3d9e510},
//     {.sector = 23, .key_a = 0x9d993c5d4ef4, .key_b = 0x86120e488abf},
//     {.sector = 24, .key_a = 0xc65d4eaa645b, .key_b = 0xb69d40d1a439},
//     {.sector = 25, .key_a = 0x3a8a139c20b4, .key_b = 0x8818a9c5d406},
//     {.sector = 26, .key_a = 0xbaff3053b496, .key_b = 0x4b7cb25354d3},
//     {.sector = 27, .key_a = 0x7413b599c4ea, .key_b = 0xb0a2aaf3a1ba},
//     {.sector = 28, .key_a = 0x0ce7cd2cc72b, .key_b = 0xfa1fbb3f0f1f},
//     {.sector = 29, .key_a = 0x0be5fac8b06a, .key_b = 0x6f95887a4fd3},
//     {.sector = 30, .key_a = 0x0eb23cc8110b, .key_b = 0x04dc35277635},
//     {.sector = 31, .key_a = 0xbc4580b7f20b, .key_b = 0xd0a4131fb290},
//     {.sector = 32, .key_a = 0x7a396f0d633d, .key_b = 0xad2bdc097023},
//     {.sector = 33, .key_a = 0xa3faa6daff67, .key_b = 0x7600e889adf9},
//     {.sector = 34, .key_a = 0xfd8705e721b0, .key_b = 0x296fc317a513},
//     {.sector = 35, .key_a = 0x22052b480d11, .key_b = 0xe19504c39461},
//     {.sector = 36, .key_a = 0xa7141147d430, .key_b = 0xff16014fefc7},
//     {.sector = 37, .key_a = 0x8a8d88151a00, .key_b = 0x038b5f9b5a2a},
//     {.sector = 38, .key_a = 0xb27addfb64b0, .key_b = 0x152fd0c420a7},
//     {.sector = 39, .key_a = 0x7259fa0197c6, .key_b = 0x5583698df085},
// };

//keys for 4k
// 000 | 003 | A0A1A2A3A4A5 | D | B0B1B2B3B4B5
// 001 | 007 | 000000000000 | D | 7DDF9E3F4020
// 002 | 011 | A0A1A2A3A4A5 | D | B0B1B2B3B4B5
// 003 | 015 | A0A1A2A3A4A5 | D | B0B1B2B3B4B5
// 004 | 019 | FFFFFFFFFFFF | D | FFFFFFFFFFFF
// 005 | 023 | FFFFFFFFFFFF | D | FFFFFFFFFFFF
// 006 | 027 | 000000000000 | D | 2B780F1ED75B
// 007 | 031 | 2066F4727129 | D | F7A65799C6EE
// 008 | 035 | 26973EA74321 | D | D27058C6E2C7
// 009 | 039 | EB0A8FF88ADE | D | 578A9ADA41E3
// 010 | 043 | EA0FD73CB149 | D | 29C35FA068FB
// 011 | 047 | C76BF71A2509 | D | 9BA241DB3F56
// 012 | 051 | 000000000000 | D | 71F3A315AD26
// 013 | 055 | AC70CA327A04 | D | F29411C2663C
// 014 | 059 | 51044EFB5AAB | D | EBDC720DD1CE
// 015 | 063 | A0A1A2A3A4A5 | D | 103C08ACCEB2
// 016 | 067 | 72F96BDD3714 | D | 462225CD34CF
// 017 | 071 | 044CE1872BC3 | D | 8C90C70CFF4A
// 018 | 075 | BC2D1791DEC1 | D | CA96A487DE0B
// 019 | 079 | 8791B2CCB5C4 | D | C956C3B80DA3
// 020 | 083 | 8E26E45E7D65 | D | 8E65B3AF7D22
// 021 | 087 | 0F318130ED18 | D | 0C420A20E056
// 022 | 091 | 045CECA15535 | D | 31BEC3D9E510
// 023 | 095 | 9D993C5D4EF4 | D | 86120E488ABF
// 024 | 099 | C65D4EAA645B | D | B69D40D1A439
// 025 | 103 | 46D78E850A7E | H | A470F8130991
// 026 | 107 | 42E9B54E51AB | H | 0231B86DF52E
// 027 | 111 | 0F01CEFF2742 | H | 6FEC74559CA7
// 028 | 115 | B81F2B0C2F66 | H | A7E2D95F0003
// 029 | 119 | 9EA3387A63C1 | H | 437E59F57561
// 030 | 123 | 0EB23CC8110B | D | 04DC35277635
// 031 | 127 | BC4580B7F20B | D | D0A4131FB290
// 032 | 143 | 7A396F0D633D | D | AD2BDC097023
// 033 | 159 | A3FAA6DAFF67 | D | 7600E889ADF9
// 034 | 175 | FD8705E721B0 | D | 296FC317A513
// 035 | 191 | 22052B480D11 | D | E19504C39461
// 036 | 207 | A7141147D430 | D | FF16014FEFC7
// 037 | 223 | 8A8D88151A00 | D | 038B5F9B5A2A
// 038 | 239 | B27ADDFB64B0 | D | 152FD0C420A7
// 039 | 255 | 7259FA0197C6 | D | 5583698DF085

static const MfClassicAuthContext concession_spb_keys_4k[] = {
    {.sector = 0, .key_a = 0xa0a1a2a3a4a5, .key_b = 0xb0b1b2b3b4b5},
    {.sector = 1, .key_a = 0x000000000000, .key_b = 0x7ddf9e3f4020},
    {.sector = 2, .key_a = 0xa0a1a2a3a4a5, .key_b = 0xb0b1b2b3b4b5},
    {.sector = 3, .key_a = 0xa0a1a2a3a4a5, .key_b = 0xb0b1b2b3b4b5},
    {.sector = 4, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 5, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
    {.sector = 6, .key_a = 0x000000000000, .key_b = 0x2b780f1ed75b},
    {.sector = 7, .key_a = 0x2066f4727129, .key_b = 0xf7a65799c6ee},
    {.sector = 8, .key_a = 0x26973ea74321, .key_b = 0xd27058c6e2c7},
    {.sector = 9, .key_a = 0xeb0a8ff88ade, .key_b = 0x578a9ada41e3},
    {.sector = 10, .key_a = 0xea0fd73cb149, .key_b = 0x29c35fa068fb},
    {.sector = 11, .key_a = 0xc76bf71a2509, .key_b = 0x9ba241db3f56},
    {.sector = 12, .key_a = 0x000000000000, .key_b = 0x71f3a315ad26},
    {.sector = 13, .key_a = 0xac70ca327a04, .key_b = 0xf29411c2663c},
    {.sector = 14, .key_a = 0x51044efb5aab, .key_b = 0xebdc720dd1ce},
    {.sector = 15, .key_a = 0xa0a1a2a3a4a5, .key_b = 0x103c08acceb2},
    {.sector = 16, .key_a = 0x72f96bdd3714, .key_b = 0x462225cd34cf},
    {.sector = 17, .key_a = 0x044ce1872bc3, .key_b = 0x8c90c70cff4a},
    {.sector = 18, .key_a = 0xbc2d1791dec1, .key_b = 0xca96a487de0b},
    {.sector = 19, .key_a = 0x8791b2ccb5c4, .key_b = 0xc956c3b80da3},
    {.sector = 20, .key_a = 0x8e26e45e7d65, .key_b = 0x8e65b3af7d22},
    {.sector = 21, .key_a = 0x0f318130ed18, .key_b = 0x0c420a20e056},
    {.sector = 22, .key_a = 0x045ceca15535, .key_b = 0x31bec3d9e510},
    {.sector = 23, .key_a = 0x9d993c5d4ef4, .key_b = 0x86120e488abf},
    {.sector = 24, .key_a = 0xc65d4eaa645b, .key_b = 0xb69d40d1a439},
    {.sector = 25, .key_a = 0x46d78e850a7e, .key_b = 0xa470f8130991},
    {.sector = 26, .key_a = 0x42e9b54e51ab, .key_b = 0x0231b86df52e},
    {.sector = 27, .key_a = 0x0f01ceff2742, .key_b = 0x6fec74559ca7},
    {.sector = 28, .key_a = 0xb81f2b0c2f66, .key_b = 0xa7e2d95f0003},
    {.sector = 29, .key_a = 0x9ea3387a63c1, .key_b = 0x437e59f57561},
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

// static const MfClassicAuthContext concession_spb_keys_4k_1[] = {
//     {.sector = 0, .key_a = 0xa0a1a2a3a4a5, .key_b = 0xb0b1b2b3b4b5},
//     {.sector = 1, .key_a = 0x000000000000, .key_b = 0x7ddf9e3f4020},
//     {.sector = 2, .key_a = 0xa0a1a2a3a4a5, .key_b = 0xb0b1b2b3b4b5},
//     {.sector = 3, .key_a = 0xa0a1a2a3a4a5, .key_b = 0xb0b1b2b3b4b5},
//     {.sector = 4, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
//     {.sector = 5, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
//     {.sector = 6, .key_a = 0x000000000000, .key_b = 0x2b780f1ed75b},
//     {.sector = 7, .key_a = 0x2066f4727129, .key_b = 0xf7a65799c6ee},
//     {.sector = 8, .key_a = 0x26973ea74321, .key_b = 0xd27058c6e2c7},
//     {.sector = 9, .key_a = 0xeb0a8ff88ade, .key_b = 0x578a9ada41e3},
//     {.sector = 10, .key_a = 0xea0fd73cb149, .key_b = 0x29c35fa068fb},
//     {.sector = 11, .key_a = 0xc76bf71a2509, .key_b = 0x9ba241db3f56},
//     {.sector = 12, .key_a = 0x000000000000, .key_b = 0x71f3a315ad26},
//     {.sector = 13, .key_a = 0xac70ca327a04, .key_b = 0xf29411c2663c},
//     {.sector = 14, .key_a = 0x51044efb5aab, .key_b = 0xebdc720dd1ce},
//     {.sector = 15, .key_a = 0xa0a1a2a3a4a5, .key_b = 0x103c08acceb2},
//     {.sector = 16, .key_a = 0x72f96bdd3714, .key_b = 0x462225cd34cf},
//     {.sector = 17, .key_a = 0x044ce1872bc3, .key_b = 0x8c90c70cff4a},
//     {.sector = 18, .key_a = 0xbc2d1791dec1, .key_b = 0xca96a487de0b},
//     {.sector = 19, .key_a = 0x8791b2ccb5c4, .key_b = 0xc956c3b80da3},
//     {.sector = 20, .key_a = 0x8e26e45e7d65, .key_b = 0x8e65b3af7d22},
//     {.sector = 21, .key_a = 0x0f318130ed18, .key_b = 0x0c420a20e056},
//     {.sector = 22, .key_a = 0x045ceca15535, .key_b = 0x31bec3d9e510},
//     {.sector = 23, .key_a = 0x9d993c5d4ef4, .key_b = 0x86120e488abf},
//     {.sector = 24, .key_a = 0xc65d4eaa645b, .key_b = 0xb69d40d1a439},
//     {.sector = 25, .key_a = 0x46d78e850a7e, .key_b = 0xa470f8130991},
//     {.sector = 26, .key_a = 0x42e9b54e51ab, .key_b = 0x0231b86df52e},
//     {.sector = 27, .key_a = 0x0f01ceff2742, .key_b = 0x6fec74559ca7},
//     {.sector = 28, .key_a = 0xb81f2b0c2f66, .key_b = 0xa7e2d95f0003},
//     {.sector = 29, .key_a = 0x9ea3387a63c1, .key_b = 0x437e59f57561},
//     {.sector = 30, .key_a = 0x0eb23cc8110b, .key_b = 0x04dc35277635},
//     {.sector = 31, .key_a = 0xbc4580b7f20b, .key_b = 0xd0a4131fb290},
//     {.sector = 32, .key_a = 0x7a396f0d633d, .key_b = 0xad2bdc097023},
//     {.sector = 33, .key_a = 0xa3faa6daff67, .key_b = 0x7600e889adf9},
//     {.sector = 34, .key_a = 0xfd8705e721b0, .key_b = 0x296fc317a513},
//     {.sector = 35, .key_a = 0x22052b480d11, .key_b = 0xe19504c39461},
//     {.sector = 36, .key_a = 0xa7141147d430, .key_b = 0xff16014fefc7},
//     {.sector = 37, .key_a = 0x8a8d88151a00, .key_b = 0x038b5f9b5a2a},
//     {.sector = 38, .key_a = 0xb27addfb64b0, .key_b = 0x152fd0c420a7},
//     {.sector = 39, .key_a = 0x7259fa0197c6, .key_b = 0x5583698df085},
// };

// static const MfClassicAuthContext concession_spb_keys_4k_2[] = {
//     {.sector = 0, .key_a = 0xa0a1a2a3a4a5, .key_b = 0xb0b1b2b3b4b5},
//     {.sector = 1, .key_a = 0xd3f7d3f7d3f7, .key_b = 0xffffffffffff},
//     {.sector = 2, .key_a = 0xa0a1a2a3a4a5, .key_b = 0xb0b1b2b3b4b5},
//     {.sector = 3, .key_a = 0xa0a1a2a3a4a5, .key_b = 0xb0b1b2b3b4b5},
//     {.sector = 4, .key_a = 0xe56ac127dd45, .key_b = 0x19fc84a3784b},
//     {.sector = 5, .key_a = 0x77dabc9825e1, .key_b = 0x9764fec3154a},
//     {.sector = 6, .key_a = 0xffffffffffff, .key_b = 0xffffffffffff},
//     {.sector = 7, .key_a = 0x2066f4727129, .key_b = 0xf7a65799c6ee},
//     {.sector = 8, .key_a = 0x26973ea74321, .key_b = 0xd27058c6e2c7},
//     {.sector = 9, .key_a = 0xeb0a8ff88ade, .key_b = 0x578a9ada41e3},
//     {.sector = 10, .key_a = 0xea0fd73cb149, .key_b = 0x29c35fa068fb},
//     {.sector = 11, .key_a = 0xc76bf71a2509, .key_b = 0x9ba241db3f56},
//     {.sector = 12, .key_a = 0x000000000000, .key_b = 0x71f3a315ad26},
//     {.sector = 13, .key_a = 0xac70ca327a04, .key_b = 0xf29411c2663c},
//     {.sector = 14, .key_a = 0x51044efb5aab, .key_b = 0xebdc720dd1ce},
//     {.sector = 15, .key_a = 0xa0a1a2a3a4a5, .key_b = 0x103c08acceb2},
//     {.sector = 16, .key_a = 0x72f96bdd3714, .key_b = 0x462225cd34cf},
//     {.sector = 17, .key_a = 0x044ce1872bc3, .key_b = 0x8c90c70cff4a},
//     {.sector = 18, .key_a = 0xbc2d1791dec1, .key_b = 0xca96a487de0b},
//     {.sector = 19, .key_a = 0x8791b2ccb5c4, .key_b = 0xc956c3b80da3},
//     {.sector = 20, .key_a = 0x8e26e45e7d65, .key_b = 0x8e65b3af7d22},
//     {.sector = 21, .key_a = 0x0f318130ed18, .key_b = 0x0c420a20e056},
//     {.sector = 22, .key_a = 0x045ceca15535, .key_b = 0x31bec3d9e510},
//     {.sector = 23, .key_a = 0x9d993c5d4ef4, .key_b = 0x86120e488abf},
//     {.sector = 24, .key_a = 0xc65d4eaa645b, .key_b = 0xb69d40d1a439},
//     {.sector = 25, .key_a = 0x3a8a139c20b4, .key_b = 0x8818a9c5d406},
//     {.sector = 26, .key_a = 0xbaff3053b496, .key_b = 0x4b7cb25354d3},
//     {.sector = 27, .key_a = 0x7413b599c4ea, .key_b = 0xb0a2aaf3a1ba},
//     {.sector = 28, .key_a = 0x0ce7cd2cc72b, .key_b = 0xfa1fbb3f0f1f},
//     {.sector = 29, .key_a = 0x0be5fac8b06a, .key_b = 0x6f95887a4fd3},
//     {.sector = 30, .key_a = 0x0eb23cc8110b, .key_b = 0x04dc35277635},
//     {.sector = 31, .key_a = 0xbc4580b7f20b, .key_b = 0xd0a4131fb290},
//     {.sector = 32, .key_a = 0x7a396f0d633d, .key_b = 0xad2bdc097023},
//     {.sector = 33, .key_a = 0xa3faa6daff67, .key_b = 0x7600e889adf9},
//     {.sector = 34, .key_a = 0xfd8705e721b0, .key_b = 0x296fc317a513},
//     {.sector = 35, .key_a = 0x22052b480d11, .key_b = 0xe19504c39461},
//     {.sector = 36, .key_a = 0xa7141147d430, .key_b = 0xff16014fefc7},
//     {.sector = 37, .key_a = 0x8a8d88151a00, .key_b = 0x038b5f9b5a2a},
//     {.sector = 38, .key_a = 0xb27addfb64b0, .key_b = 0x152fd0c420a7},
//     {.sector = 39, .key_a = 0x7259fa0197c6, .key_b = 0x5583698df085},
// };

// const MfClassicAuthContext* all_keys[] = {
//     concession_spb_keys_4k_2,
//     concession_spb_keys_4k_0,
//     concession_spb_keys_4k_1};

bool concession_spb_4k_parser_verify_1(NfcWorker* nfc_worker, FuriHalNfcTxRxContext* tx_rx) {
    furi_assert(nfc_worker);
    UNUSED(nfc_worker);

    if(nfc_worker->dev_data->mf_classic_data.type != MfClassicType4k) {
        return false;
    }

    uint8_t sector = 8;
    uint8_t block = mf_classic_get_sector_trailer_block_num_by_sector(sector);
    FURI_LOG_D("conc4k", "Verifying sector %d", sector);
    if(mf_classic_authenticate(tx_rx, block, 0x26973ea74321, MfClassicKeyA)) {
        FURI_LOG_D("conc4k", "Sector %d verified", sector);
        return true;
    }
    return false;
}

bool concession_spb_4k_parser_read_1(NfcWorker* nfc_worker, FuriHalNfcTxRxContext* tx_rx) {
    furi_assert(nfc_worker);

    MfClassicReader reader = {};
    FuriHalNfcDevData* nfc_data = &nfc_worker->dev_data->nfc_data;
    reader.type = mf_classic_get_classic_type(nfc_data->atqa[0], nfc_data->atqa[1], nfc_data->sak);
    // size_t keys_num = 3; //TODO: calc keys_num
    // for (size_t keys_index = 0; keys_index < keys_num; keys_index++) {
    for(size_t i = 0; i < COUNT_OF(concession_spb_keys_4k); i++) {
        // FURI_LOG_D("conc4k", "Iter %d", keys_index);
        // for(size_t i = 0; i < ; i++) {
        mf_classic_reader_add_sector(
            &reader,
            concession_spb_keys_4k[i].sector,
            concession_spb_keys_4k[i].key_a,
            concession_spb_keys_4k[i].key_b);
        FURI_LOG_T("conc4k", "Added sector %d", concession_spb_keys_4k[i].sector);
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

bool concession_spb_4k_parser_parse_1(NfcDeviceData* dev_data) {
    MfClassicData* data = &dev_data->mf_classic_data;

    // Verify key
    MfClassicSectorTrailer* sec_tr = mf_classic_get_sector_trailer_by_sector(data, 8);
    uint64_t key = nfc_util_bytes2num(sec_tr->key_a, 6);
    if(key != concession_spb_keys_4k[8].key_a) return false;

    // Point to block 0 of sector 4, value 0
    // uint8_t* temp_ptr = &data->block[4 * 4].value[0];
    // Read first 4 bytes of block 0 of sector 4 from last to first and convert them to uint32_t
    // 38 18 00 00 becomes 00 00 18 38, and equals to 6200 decimal
    // uint32_t balance =
    //     ((temp_ptr[3] << 24) | (temp_ptr[2] << 16) | (temp_ptr[1] << 8) | temp_ptr[0]) / 100;

    // Read passport series from block 1 of sector 8, bytes 3-8

    // Point to block 1 of sector 8
    uint8_t* temp_ptr = &data->block[8 * 4 + 1].value[0];
    // Read bytes 3-8 of block 1 of sector 8
    string_t passport_series;
    string_init_printf(
        passport_series, "%c%c%c%c", temp_ptr[3], temp_ptr[4], temp_ptr[6], temp_ptr[7]);

    // Read passport number from block 1 of sector 8, bytes 9-12, liile endian

    // Point to block 1 of sector 8
    temp_ptr = &data->block[8 * 4 + 1].value[0];
    // Read bytes 9-11 of block 1 of sector 8
    uint32_t passport_number = (temp_ptr[11] << 16) | (temp_ptr[10] << 8) | temp_ptr[9];

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
    string_t card_number_str;
    string_init(card_number_str);
    // Should look like "361301047292848684"
    // %llu doesn't work for some reason in sprintf, so we use string_push_uint64 instead
    string_push_uint64(card_number, card_number_str);
    // Add suffix with luhn checksum (1 digit) to the card number string
    string_t card_number_suffix;
    string_init(card_number_suffix);

    // The number to calculate the checksum on doesn't fit into uint64_t, idk
    //uint8_t luhn_checksum = plantain_calculate_luhn(card_number);

    // // Convert luhn checksum to string
    // string_t luhn_checksum_str;
    // string_init(luhn_checksum_str);
    // string_push_uint64(luhn_checksum, luhn_checksum_str);

    string_cat_printf(card_number_suffix, "-");
    // FURI_LOG_D("conc4k", "Card checksum: %d", luhn_checksum);
    string_cat_printf(card_number_str, string_get_cstr(card_number_suffix));
    // Free all not needed strings
    string_clear(card_number_suffix);
    // string_clear(luhn_checksum_str);

    string_printf(
        dev_data->parsed_data,
        "\e#Concession_1\nN:%s\nPN:%s %d",
        string_get_cstr(card_number_str),
        string_get_cstr(passport_series),
        passport_number);
    string_clear(card_number_str);

    return true;
}
