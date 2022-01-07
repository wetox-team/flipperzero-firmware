#include "mifare_classic.h"
#include <furi.h>
#include <furi_hal.h>
#include <stdlib.h>

bool mf_plus_check_card_type(uint8_t ATQA0, uint8_t ATQA1, uint8_t SAK) {
    if((ATQA0 == 0x44) && (ATQA1 == 0x00) && (SAK == 0x20)) {
        return true;
    }
    return false;
}
