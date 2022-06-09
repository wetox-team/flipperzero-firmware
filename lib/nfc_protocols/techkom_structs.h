#pragma once
#include <digital_signal.h>

typedef struct {
    uint8_t cuid[8];
} TechkomEmulator;

typedef struct {
    DigitalSignal* one;
    DigitalSignal* zero;
    DigitalSignal* tx_signal;
} TechkomSignal;
