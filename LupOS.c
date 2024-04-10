#include <furi.h>
#include <furi_hal.h>

#include <gui/gui.h>
#include <storage/storage.h>
#include <input/input.h>

#include <string.h>
#include <playerSettings.h>

#include "lupos_icons.h"
#define TAG "lupos-app"

#define LUPOS_APP_BASE_FOLDER ANY_PATH("lupos")
#define LUPOS_APP_SCRIPT_EXTENSION ".txt"

#define LUPOS_PATH LUPOS_APP_BASE_FOLDER "/map1" LUPOS_APP_SCRIPT_EXTENSION

#define LETTER_LENGTH 5
#define LETTER_HEIGHT 7

int main_map[30][3] = {{0, 0}, {0, 0}};
int player_cords[IMAGE_SIZE * IMAGE_SIZE] = {0, 0};
bool game_running = false;
bool running = true;

static void player_cords_get(int x, int y) {
    int inplayer_cords[IMAGE_SIZE * IMAGE_SIZE] = {0, 0};
    for(int ii = IMAGE_SIZE; ii >= 0; ii--) {
        for(int i = IMAGE_SIZE; i >= 0; i--) {
            inplayer_cords[ii + i] = x + ii + i;
            i--;
            inplayer_cords[ii + i] = y + ii + i;
        }
    }
    *player_cords = *inplayer_cords;
}

static void create_map() {
    char string_map[30];
    File* file = storage_file_alloc(furi_record_open(RECORD_STORAGE));
    FURI_LOG_D(TAG, "Create Map Storage Call");
    if(storage_file_open(file, LUPOS_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        FURI_LOG_D(TAG, "Create Map Storage Read");
        storage_file_read(file, &string_map, 30);
    }
    storage_file_close(file);
    storage_file_free(file);
    for(int i = 0; i <= 27; i++) {
        main_map[i][0] = string_map[i] - '0';
        main_map[i][1] = string_map[i + 1] - '0';
        main_map[i][2] = string_map[i + 2] - '0';
        while(main_map[i][0] % 18 != 0) main_map[i][0] = main_map[i][0] + 1;
        while(main_map[i][1] % 18 != 0) main_map[i][1] = main_map[i][1] + 1;
        i = i + 2;
    }
}
typedef struct {
    uint8_t x, y;
} ImagePosition;

static ImagePosition player_position = {.x = 0, .y = 0};
static ImagePosition previeous_position = {.x = 0, .y = 0};
static ImagePosition current_position = {.x = 1, .y = 1};

static bool getPositionalDrift(int positionX, int positionY, int distanceToEdges) {
    if(positionX - distanceToEdges < 0) {
        return true;
    }
    if(positionY - distanceToEdges < 0) {
        return true;
    }
    if(positionX + distanceToEdges + IMAGE_SIZE > 128) {
        return true;
    }
    if(positionY + distanceToEdges + IMAGE_SIZE > 64) {
        return true;
    }
    return false;
}

static void update_obj_position(int jumps, bool x, bool y) {
    if(x == true) {
        current_position.x = jumps + previeous_position.x;
    }
    if(y == true) {
        current_position.y = jumps + previeous_position.y;
    }
}

static void draw_correct_img(Canvas* canvas, int posx, int posy, int img) {
    if(img == 1) {
        canvas_draw_icon(canvas, posx, posy, &I_Brick18x18);
    }
    if(img == 2) {
        canvas_draw_icon(canvas, posx, posy, &I_Mesh18x18);
    }
    if(img == 3) {
        canvas_draw_icon(canvas, posx, posy, &I_Mesh18x18);
    }
}

static void redraw_background_elements(Canvas* canvas) {
    if(getPositionalDrift(player_position.x, player_position.y, 2)) {
        for(int i = 0; i <= 27; i++) {
            draw_correct_img(
                canvas,
                current_position.x - main_map[i][0] % 128,
                current_position.y - main_map[i][1] % 64,
                main_map[i][2]);
        }
        previeous_position = current_position;
    } else {
        for(int i = 0; i <= 27; i++) {
            draw_correct_img(
                canvas,
                previeous_position.x - main_map[i][0] % 128,
                previeous_position.y - main_map[i][1] % 64,
                main_map[i][2]);
        }
    }
}

static void lock_up_player(Canvas* canvas) {
    if(player_position.x - 2 < 0) {
        player_position.x = player_position.x + 4;
        canvas_draw_icon(canvas, player_position.x % 128, player_position.y % 64, &I_player);
    }
    if(player_position.x + 2 + IMAGE_SIZE > 128) {
        player_position.x = player_position.x - 4;
        canvas_draw_icon(canvas, player_position.x % 128, player_position.y % 64, &I_player);
    }
    if(player_position.y - 2 < 0) {
        player_position.y = player_position.y + 4;
        canvas_draw_icon(canvas, player_position.x % 128, player_position.y % 64, &I_player);
    }
    if(player_position.y + 2 + IMAGE_SIZE > 64) {
        player_position.y = player_position.y - 4;
        canvas_draw_icon(canvas, player_position.x % 128, player_position.y % 64, &I_player);
    } else {
        canvas_draw_icon(canvas, player_position.x % 128, player_position.y % 64, &I_player);
    }
}
int current_button = 1;
int helper = 1;
static void draw_arrow(Canvas* canvas, int x, int y) {
    if(helper >= 80) helper = 1;
    if(helper % 10 == 0) {
        canvas_draw_triangle(canvas, x, y + current_button, 7, 7, 0);
        helper = helper + 10;
    } else {
        canvas_draw_triangle(canvas, x - 3, y + current_button, 7, 7, 0);
        helper = helper + 1;
    }
}
char text[] = "  Play Game  ";
// Screen is 128x64 px
// Letters are 5x7
static void app_draw_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    canvas_clear(canvas);
    if(game_running == false) {
        char text2[] = "Quit";
        int center_x = ((LETTER_LENGTH * sizeof(text)) - sizeof(text)) / 2;
        int center_x2 = ((LETTER_LENGTH * sizeof(text2)) - sizeof(text2)) / 2;
        canvas_draw_str(canvas, 64 - center_x, 32, text);
        canvas_draw_str(canvas, 64 - center_x2, 62, text2);
        draw_arrow(canvas, 64 - center_x - 9, 32 - 3);
    } else {
        strcpy(text, "Continue Game");
        redraw_background_elements(canvas);
        lock_up_player(canvas);
        player_cords_get(player_position.x, player_position.y);
        int on_point = 0;
        for(int ii = 0; ii < 27; ii++) {
            for(int i = 0; i < IMAGE_SIZE * IMAGE_SIZE; i++) {
                if(player_cords[i] == (current_position.x - main_map[ii][0] % 128) &&
                   player_cords[i + 1] == (current_position.y - main_map[ii][1] % 64)) {
                    on_point++;
                }
                i++;
            }
        }
        if(on_point > 1) {
            game_running = false;
        }
    }
}

// also should be able to load and svae the data to make easy level saves and also for enabeling level building

static void app_input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

int32_t example_images_main(void* p) {
    UNUSED(p);
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    // Configure view port
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, app_draw_callback, view_port);
    view_port_input_callback_set(view_port, app_input_callback, event_queue);

    // Register view port in GUI
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);
    create_map();
    InputEvent event;

    while(running) {
        if(furi_message_queue_get(event_queue, &event, 100) == FuriStatusOk) {
            if((event.type == InputTypePress) || (event.type == InputTypeRepeat)) {
                if(game_running) {
                    switch(event.key) {
                    case InputKeyLeft:
                        player_position.x -= 4;
                        update_obj_position(4, true, false);
                        break;
                    case InputKeyRight:
                        player_position.x += 4;
                        update_obj_position(-4, true, false);
                        break;
                    case InputKeyUp:
                        player_position.y -= 4;
                        update_obj_position(4, false, true);
                        break;
                    case InputKeyDown:
                        player_position.y += 4;
                        update_obj_position(-4, false, true);
                        break;
                    case InputKeyBack:
                        game_running = false;
                        break;
                    case InputKeyOk:
                        break;
                    default:
                        running = false;
                        break;
                    }
                    continue;
                }
                switch(event.key) {
                case InputKeyUp:
                    current_button = 1;
                    break;
                case InputKeyDown:
                    current_button = 30;
                    break;
                case InputKeyOk:
                    if(current_button != 1) {
                        running = false;
                    } else {
                        game_running = true;
                    }
                    break;
                case InputKeyBack:
                    game_running = true;
                    break;
                default:
                    break;
                }
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