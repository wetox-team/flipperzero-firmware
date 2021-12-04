/**
 * @file furi-hal-ohs.h
 * OpenHaystack HAL API
 */

#pragma once

bool furi_hal_ohs_start();
bool furi_hal_ohs_stop();

bool furi_hal_ohs_load_key(uint8_t* key);
bool furi_hal_ohs_save_key(uint8_t* key);
