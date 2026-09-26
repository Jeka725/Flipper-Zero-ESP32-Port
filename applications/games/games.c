#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <loader/loader.h>

typedef struct {
    Gui* gui;
    ViewDispatcher* dispatcher;
    Submenu* menu;
} GamesApp;

static const char* const game_names[] = {
    "Snake",
    "Tetris",
    "Pong",
    "Game 15",
    "Snake 2.0",
};

static const char* const game_apps[] = {
    "snake",
    "tetris",
    "flipper_pong",
    "game15",
    "snake20",
};

static void games_callback(void* context, uint32_t index) {
    GamesApp* app = context;
    if(index >= COUNT_OF(game_apps)) return;

    Loader* loader = furi_record_open(RECORD_LOADER);
    loader_start_with_gui_error(loader, game_apps[index], NULL);
    furi_record_close(RECORD_LOADER);
}

static uint32_t games_exit_callback(void* context) {
    UNUSED(context);
    return VIEW_NONE;
}

int32_t games_app(void* p) {
    UNUSED(p);

    GamesApp app = {0};
    app.gui = furi_record_open(RECORD_GUI);
    app.dispatcher = view_dispatcher_alloc();
    app.menu = submenu_alloc();

    for(size_t i = 0; i < COUNT_OF(game_names); i++) {
        submenu_add_item(app.menu, game_names[i], i, games_callback, &app);
    }

    View* view = submenu_get_view(app.menu);
    view_set_previous_callback(view, games_exit_callback);
    view_dispatcher_add_view(app.dispatcher, 0, view);
    view_dispatcher_attach_to_gui(app.dispatcher, app.gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_run(app.dispatcher);

    view_dispatcher_remove_view(app.dispatcher, 0);
    submenu_free(app.menu);
    view_dispatcher_free(app.dispatcher);
    furi_record_close(RECORD_GUI);

    return 0;
}
