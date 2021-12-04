#pragma once

typedef struct{
    uint8_t key[28];
} Ohs_key;

bool ohs_key_load(Ohs_key* ohs_key);
bool ohs_key_save(Ohs_key* ohs_key);
