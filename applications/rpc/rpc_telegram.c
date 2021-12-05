#include "flipper.pb.h"
#include "rpc_i.h"
#include "telegram.pb.h"
#include "rpc_telegram.h"

#define TAG "RpcTG"

typedef struct {
    Rpc* rpc;
    TelegramApi* api;
} RpcTGSystem;

static void state_callback(const PB_Main* request, void* context) {
    RpcTGSystem* rpcSystem = context;
    rpcSystem->api->handler(&request->content.tg_state_response, rpcSystem->api->instance);
}

void rpc_get_state_request(Rpc* rpc) {
    //TelegramApi* tg_api = furi_record_open("tg");
    PB_Telegram_TelegramStateRequest innerRequest = {
        .count = 3,
    };

    PB_Main request = {
        .command_id = 1,
        .has_next = false,
        .which_content = PB_Main_tg_state_request_tag,
        .command_status = PB_CommandStatus_OK,
    };

    request.content.tg_state_request = innerRequest;

    //FURI_LOG_I(TAG, request);
    rpc_send_and_release(rpc, &request);
}

void* rpc_tg_alloc(Rpc* rpc) {
    furi_assert(rpc);

    RpcTGSystem* rpc_tg = furi_alloc(sizeof(RpcTGSystem));
    TelegramApi* tg_api = furi_alloc(sizeof(TelegramApi));
    furi_record_create("tg", tg_api);
    
    rpc_tg->rpc = rpc;
    rpc_tg->api = tg_api;
    tg_api->get_state = rpc_get_state_request;
    tg_api->rpc = rpc;

    RpcHandler rpc_handler = {
        .message_handler = NULL,
        .decode_submessage = NULL,
        .context = rpc_tg,
    };

    rpc_handler.message_handler = state_callback;
    rpc_add_handler(rpc, PB_Main_tg_state_response_tag, &rpc_handler);

    return rpc_tg;
}

void rpc_tg_free(void* ctx) {
    furi_assert(ctx);
    RpcTGSystem* rpc_tg = ctx;
    furi_assert(rpc_tg->api);

    furi_record_destroy("tg");
    free(rpc_tg->api);
    free(rpc_tg);
}