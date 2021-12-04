#include "telegram.pb.h"

typedef void (*TGStateHandler)(const PB_Telegram_TelegramStateResponse dialogs);

typedef struct {
    TGStateHandler handler;
} TelegramApi;