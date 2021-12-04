
#include "telegram.pb.h"

typedef struct {
    FuriPubSub* sendQueue;
    FuriPubSub* recvQueue;
} TelegramApi;

PB_Telegram_TelegramStateResponse* get_state(TelegramApi* api);