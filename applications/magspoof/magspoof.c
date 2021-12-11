#include "magspoof.h"
#include "magspoof_device.h"
#include "magspoof_i.h"

// static uint32_t magspoof_exit(void* context) {
//     return VIEW_NONE;
// }

bool magspoof_back_event_callback(void* context) {
    furi_assert(context);
    Magspoof* app = (Magspoof*)context;
    return scene_manager_handle_back_event(app->scene_manager);
}

bool magspoof_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    Magspoof* app = (Magspoof*)context;
    return scene_manager_handle_custom_event(app->scene_manager, event);
}

static Magspoof* magspoof_alloc() {
    Magspoof* app = furi_alloc(sizeof(Magspoof));
    // UartDumpModel* cont = furi_alloc(sizeof(UartDumpModel));

    app->rx_stream = xStreamBufferCreate(2048, 1);

    // Gui
    app->gui = furi_record_open("gui");
    app->notification = furi_record_open("notification");

    // View dispatcher
    app->view_dispatcher = view_dispatcher_alloc();
    app->scene_manager = scene_manager_alloc(&magspoof_scene_handlers, app);
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, magspoof_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, magspoof_back_event_callback);

    // Views
    // app->view = view_alloc();
     // view_set_draw_callback(app->view, magspoof_view_draw_callback);
     // view_set_input_callback(app->view, magspoof_view_input_callback);
    
    // view_allocate_model(app->view, ViewModelTypeLocking, sizeof(UartDumpModel));

    // view_set_context(app->view, app);

    // with_view_model(
    //     app->view, (UartDumpModel * model) {
    //         for(size_t i = 0; i < LINES_ON_SCREEN; i++) {
    //             model->line = 0;
    //             model->escape = false;
    //             model->list[i] = furi_alloc(sizeof(ListElement));
    //             string_init(model->list[i]->text);
    //         }
    //         return true;
    //     });


    // Submenu
    app->submenu = submenu_alloc();
    view_dispatcher_add_view(app->view_dispatcher, MagspoofViewMenu, submenu_get_view(app->submenu));


    // Dialog
    app->dialog_ex = dialog_ex_alloc();
    view_dispatcher_add_view(
        app->view_dispatcher, MagspoofViewDialogEx, dialog_ex_get_view(app->dialog_ex));



    // view_set_previous_callback(app->view, magspoof_exit);

    

    // view_dispatcher_add_view(app->view_dispatcher, 0, app->view);
    view_dispatcher_switch_to_view(app->view_dispatcher, MagspoofViewMenu);

    // Enable uart listener
    furi_hal_console_disable();
    // furi_hal_uart_set_br(FuriHalUartIdUSART1, 9600);
    // furi_hal_uart_set_irq_cb(FuriHalUartIdUSART1, magspoof_on_irq_cb, app);

    // app->worker_thread = furi_thread_alloc();
    // furi_thread_set_name(app->worker_thread, "MagspoofWorker");
    // furi_thread_set_stack_size(app->worker_thread, 1024);
    // furi_thread_set_context(app->worker_thread, app);
    // furi_thread_set_callback(app->worker_thread, magspoof_worker);
    // furi_thread_start(app->worker_thread);

    app->dev = magspoof_device_alloc();


    return app;
}

static void magspoof_free(Magspoof* app) {
    furi_assert(app);

    // osThreadFlagsSet(furi_thread_get_thread_id(app->worker_thread), WorkerEventStop);
    // furi_thread_join(app->worker_thread);
    // furi_thread_free(app->worker_thread);

    magspoof_device_free(app->dev);

    furi_hal_console_enable();

    // Free views
    view_dispatcher_remove_view(app->view_dispatcher, MagspoofViewMenu);
    submenu_free(app->submenu);

    view_dispatcher_remove_view(app->view_dispatcher, MagspoofViewDialogEx);
    dialog_ex_free(app->dialog_ex);

    // view_dispatcher_remove_view(app->view_dispatcher, 0);

    // with_view_model(
    //     app->view, (UartDumpModel * model) {
    //         for(size_t i = 0; i < LINES_ON_SCREEN; i++) {
    //             string_clear(model->list[i]->text);
    //             free(model->list[i]);
    //         }
    //         return true;
    //     });
    // view_free(app->view);

    view_dispatcher_free(app->view_dispatcher);

    scene_manager_free(app->scene_manager);

    // Close gui record
    furi_record_close("gui");
    furi_record_close("notification");
    app->gui = NULL;

    vStreamBufferDelete(app->rx_stream);

    // Free rest
    free(app);
}

int32_t magspoof_app(void* p) {
    Magspoof* app = magspoof_alloc();

    scene_manager_next_scene(app->scene_manager, MagspoofSceneStart);

    view_dispatcher_run(app->view_dispatcher);
    magspoof_free(app);
    return 0;
}