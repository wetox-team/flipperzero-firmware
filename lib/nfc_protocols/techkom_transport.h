#pragma once

typedef struct {
    uint8_t cuid[8];
} TechkomEmulator;

typedef struct {
    DigitalSignal* one;
    DigitalSignal* end_one;
    DigitalSignal* zero;
    DigitalSignal* end_zero;
    DigitalSignal* end;
    DigitalSignal* tx_signal;
} TechkomSignal;
