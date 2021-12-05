#include "telegram.pb.h"
#include "rpc_i.h"

typedef void (*TGStateHandler)(const PB_Telegram_TelegramStateResponse* dialogs, void* instance);
typedef void (*TGStateGetter)(Rpc* rpc);
typedef void (*TGMessageSender)(Rpc* rpc, int32_t id, char* message);

typedef struct {
    TGStateHandler handler;
    void* instance;
    TGStateGetter get_state;
    TGMessageSender send_msg;

    Rpc* rpc;
} TelegramApi;
