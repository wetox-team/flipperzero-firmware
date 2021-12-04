#include "telegram.pb.h"

typedef void (*TGStateHandler)(const PB_Telegram_TelegramStateResponse* dialogs, void* instance);

typedef struct {
    TGStateHandler handler;
    void* instance;
} TelegramApi;