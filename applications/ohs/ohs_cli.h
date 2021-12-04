//
// Created by kinjalik on 12/4/21.
//
#include <cli/cli.h>

#ifndef FLIPPERZERO_FIRMWARE_OHS_CLI_H
#define FLIPPERZERO_FIRMWARE_OHS_CLI_H

void ohs_cli_init();
void ohs_cli_command(Cli* cli, string_t args, void* context);
void ohs_cli_stop(Cli* cli, string_t args, void* context);

typedef struct{
    uint8_t key[28];
} Ohs_key;

bool ohs_key_load(Ohs_key* ohs_key);
bool ohs_key_save(Ohs_key* ohs_key);
#endif //FLIPPERZERO_FIRMWARE_OHS_CLI_H
