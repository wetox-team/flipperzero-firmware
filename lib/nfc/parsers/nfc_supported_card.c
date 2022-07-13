#include "nfc_supported_card.h"

#include "plantain_parser.h"
#include "troyka_parser.h"
#include "plantain_4k_parser.h"

NfcSupportedCard nfc_supported_card[NfcSupportedCardTypeEnd] = {
    [NfcSupportedCardTypePlantain] =
        {
            .protocol = NfcDeviceProtocolMifareClassic,
            .verify = plantain_parser_verify,
            .read = plantain_parser_read,
            .parse = plantain_parser_parse,
        },
    [NfcSupportedCardTypeTroyka] =
        {
            .protocol = NfcDeviceProtocolMifareClassic,
            .verify = troyka_parser_verify,
            .read = troyka_parser_read,
            .parse = troyka_parser_parse,
        },
    [NfcSupportedCardTypePlantain4K] =
        {
            .protocol = NfcDeviceProtocolMifareClassic,
            .verify = plantain_4k_parser_verify,
            .read = plantain_4k_parser_read,
            .parse = plantain_4k_parser_parse,
        },
};
