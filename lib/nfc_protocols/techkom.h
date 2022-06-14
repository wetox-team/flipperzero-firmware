#pragma once
#include <digital_signal.h>
#include <techkom_transport.h>
#include <furi_hal_nfc.h>

#define ZERO 0
#define ONE 1
#define ONE_END 2
#define ZERO_END 3
#define QUIET 4

#define PAUSE_TIM 108
#define ONE_FIRST 1139
#define ONE_SECOND 569
#define ONE_SECOND_END 692
#define ZERO_FIRST 406
#define ZERO_SECOND 1274
#define ZERO_SECOND_END 1410
#define QUIET_PERIOD 40000

TechkomSignal* techkom_signal_alloc();

void techkom_signal_free(TechkomSignal* techkom_signal);

void techkom_emulator(TechkomEmulator* emulator, FuriHalNfcTxRxContext* tx_rx);

void techkom_add_bit(DigitalSignal* signal, uint8_t bit);

void techkom_signal_encode(TechkomSignal* techkom_signal, uint8_t* data);
