#include <furi.h>
#include <furi_hal.h>

#include <gui/gui.h>
#include <storage/storage.h>
#include <input/input.h>

#include <string.h>

#include "lupos_icons.h"
#define TAG "lupos-app"

#define LUPOS_APP_BASE_FOLDER ANY_PATH("lupos")
#define LUPOS_APP_SCRIPT_EXTENSION ".txt"

#define LUPOS_PATH LUPOS_APP_BASE_FOLDER "/map1.txt"

// Screen is 128x64 px
static void app_draw_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    canvas_clear(canvas);
}

// also should be able to load and svae the data to make easy level saves and also for enabeling level building

static void app_input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);

    FuriMessageQueue* event_queue = ctx;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

int32_t example_images_main2(void* p) {
    UNUSED(p);
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    // Configure view port
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, app_draw_callback, view_port);
    view_port_input_callback_set(view_port, app_input_callback, event_queue);

    // Register view port in GUI
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);
    InputEvent event;

    bool running = true;
    while(running) {
        if(furi_message_queue_get(event_queue, &event, 100) == FuriStatusOk) {
            if((event.type == InputTypePress) || (event.type == InputTypeRepeat)) {
            }
        }
        view_port_update(view_port);
    }

    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);

    furi_record_close(RECORD_GUI);

    return 0;
}