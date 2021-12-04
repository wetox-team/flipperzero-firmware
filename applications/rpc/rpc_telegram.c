#include "flipper.pb.h"
#include "rpc_i.h"
#include "telegram.pb.h"
#include "rpc_telegram.h"

#define TAG "RpcTG"

typedef struct {
    Rpc* rpc;
    TelegramApi* api;
} RpcTGSystem;

void* rpc_tg_alloc(Rpc* rpc) {
    furi_assert(rpc);

    RpcTGSystem* rpc_tg = furi_alloc(sizeof(RpcTGSystem));
    TelegramApi* tg_api = furi_alloc(sizeof(TelegramApi));
    furi_record_create("tg", tg_api);
    
    furi_pubsub_alloc(tg_api->recvQueue);
    furi_pubsub_alloc(tg_api->sendQueue);
    tg_api->recvQueue 
    rpc_tg->rpc = rpc;
    rpc_tg->api = tg_api;

    /*
    RpcHandler rpc_handler = {
        .message_handler = NULL,
        .decode_submessage = NULL,
        .context = rpc_tg,
    };
    */

    return rpc_tg;
}

PB_Telegram_TelegramStateResponse* get_state(TelegramApi* api) {
    PB_Main* req = furi_alloc(sizeof(PB_Main));
    req->which_content = PB_Main_tg_state_request_tag;
    req->content.tg_state_request.count = 3;

    furi_pubsub_subscribe(api->recvQueue);
    furi_pubsub_publish(api->sendQueue, req);
}

void rpc_tg_free(void* ctx) {
    furi_assert(ctx);
    RpcTGSystem* rpc_tg = ctx;
    furi_assert(rpc_tg->api);

    furi_pubsub_free(rpc_tg->api->sendQueue);
    furi_pubsub_free(rpc_tg->api->recvQueue);
    furi_record_destroy("tg");
    free(rpc_tg->api);
    free(rpc_tg);
}