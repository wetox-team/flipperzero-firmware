#pragma once

typedef enum {
    TomoEventTypeTick,
    TomoEventTypeInput,
} TomoEventType;

typedef struct {
    TomoEventType type;
    InputEvent input;
    uint32_t code;
} TomoEvent;