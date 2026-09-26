#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <input/input.h>
#include <stdlib.h>
#include <string.h>

#define GAMES_COUNT 4
#define GAMES_FIRMWARE_VERSION "1.4.3"
#define CELL 4
#define SNAKE_COLS 32
#define SNAKE_ROWS 13

typedef enum {
    GameMenu,
    GameSnake,
    GamePong,
    GameDodge,
    GameTetris,
} GameMode;

typedef struct {
    Gui* gui;
    ViewPort* viewport;
    FuriTimer* timer;
    FuriSemaphore* exit_sem;
    volatile uint32_t ticks;
    uint32_t drawn_ticks;
    GameMode mode;
    uint8_t menu_index;
    bool exit_requested;
    uint32_t score;

    int sx[96];
    int sy[96];
    int slen;
    int sdx;
    int sdy;
    int foodx;
    int foody;

    int paddle_y;
    int ball_x;
    int ball_y;
    int ball_dx;
    int ball_dy;

    int player_x;
    int rock_x;
    int rock_y;

    uint16_t board[13];
    int tx;
    int ty;
} GamesApp;

static const char* const game_names[GAMES_COUNT] = {
    "Snake",
    "Pong",
    "Dodge",
    "Tetris",
};

static void game_reset(GamesApp* app) {
    app->score = 0;
    app->drawn_ticks = app->ticks;

    if(app->mode == GameSnake) {
        app->slen = 3;
        app->sx[0] = 10;
        app->sy[0] = 5;
        app->sx[1] = 9;
        app->sy[1] = 5;
        app->sx[2] = 8;
        app->sy[2] = 5;
        app->sdx = 1;
        app->sdy = 0;
        app->foodx = 20;
        app->foody = 8;
    } else if(app->mode == GamePong) {
        app->paddle_y = 25;
        app->ball_x = 64;
        app->ball_y = 32;
        app->ball_dx = 2;
        app->ball_dy = 1;
    } else if(app->mode == GameDodge) {
        app->player_x = 60;
        app->rock_x = 35;
        app->rock_y = 12;
    } else if(app->mode == GameTetris) {
        memset(app->board, 0, sizeof(app->board));
        app->tx = 4;
        app->ty = 0;
    }
}

static bool snake_hit(GamesApp* app, int x, int y) {
    if(x < 0 || x >= SNAKE_COLS || y < 0 || y >= SNAKE_ROWS) return true;
    for(int i = 0; i < app->slen; i++) {
        if(app->sx[i] == x && app->sy[i] == y) return true;
    }
    return false;
}

static void snake_step(GamesApp* app) {
    int nx = app->sx[0] + app->sdx;
    int ny = app->sy[0] + app->sdy;

    if(snake_hit(app, nx, ny)) {
        game_reset(app);
        return;
    }

    for(int i = app->slen - 1; i > 0; i--) {
        app->sx[i] = app->sx[i - 1];
        app->sy[i] = app->sy[i - 1];
    }
    app->sx[0] = nx;
    app->sy[0] = ny;

    if(nx == app->foodx && ny == app->foody) {
        if(app->slen < 95) app->slen++;
        app->score++;
        do {
            app->foodx = rand() % SNAKE_COLS;
            app->foody = rand() % SNAKE_ROWS;
        } while(snake_hit(app, app->foodx, app->foody));
    }
}

static void pong_step(GamesApp* app) {
    app->ball_x += app->ball_dx;
    app->ball_y += app->ball_dy;

    if(app->ball_y < 12 || app->ball_y > 61) app->ball_dy = -app->ball_dy;

    if(app->ball_x < 7 && app->ball_y >= app->paddle_y &&
       app->ball_y <= app->paddle_y + 15) {
        app->ball_dx = 2;
        app->score++;
    }

    if(app->ball_x > 126) app->ball_dx = -2;
    if(app->ball_x < 0) game_reset(app);
}

static void dodge_step(GamesApp* app) {
    app->rock_y += 3;

    if(app->rock_y > 64) {
        app->rock_y = 12;
        app->rock_x = 5 + rand() % 116;
        app->score++;
    }

    if(app->rock_y + 7 >= 54 && app->rock_x < app->player_x + 10 &&
       app->rock_x + 7 > app->player_x) {
        game_reset(app);
    }
}

static void tetris_step(GamesApp* app) {
    if(app->ty < 11 &&
       !(app->board[app->ty + 2] & (1u << app->tx)) &&
       !(app->board[app->ty + 2] & (1u << (app->tx + 1)))) {
        app->ty++;
        return;
    }

    if(app->ty < 12) {
        app->board[app->ty] |= (1u << app->tx) | (1u << (app->tx + 1));
        app->board[app->ty + 1] |= (1u << app->tx) | (1u << (app->tx + 1));
    }

    for(int y = 12; y >= 0; y--) {
        if(app->board[y] == 0x03FFu) {
            for(int row = y; row > 0; row--) app->board[row] = app->board[row - 1];
            app->board[0] = 0;
            y++;
            app->score += 10;
        }
    }

    app->tx = 3 + rand() % 4;
    app->ty = 0;

    if(app->board[1] & (1u << app->tx)) game_reset(app);
}

static void game_update(GamesApp* app) {
    if(app->mode == GameSnake) {
        snake_step(app);
    } else if(app->mode == GamePong) {
        pong_step(app);
    } else if(app->mode == GameDodge) {
        dodge_step(app);
    } else if(app->mode == GameTetris) {
        tetris_step(app);
    }
}

static void draw_header(Canvas* canvas, const char* title, uint32_t score) {
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%lu", (unsigned long)score);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 8, title);
    canvas_draw_str(canvas, 110, 8, buffer);
}

static void draw_menu(Canvas* canvas, GamesApp* app) {
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 11, "Games");

    for(uint8_t i = 0; i < GAMES_COUNT; i++) {
        uint8_t y = 22 + i * 10;
        if(i == app->menu_index) {
            canvas_draw_box(canvas, 0, y - 8, 127, 10);
            canvas_set_color(canvas, ColorWhite);
            canvas_draw_str(canvas, 8, y, game_names[i]);
            canvas_set_color(canvas, ColorBlack);
        } else {
            canvas_draw_str(canvas, 8, y, game_names[i]);
        }
    }

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 62, "UP/DOWN  OK");
}

static void draw_game(Canvas* canvas, GamesApp* app) {
    canvas_clear(canvas);

    if(app->mode == GameSnake) {
        draw_header(canvas, "SNAKE", app->score);
        for(int i = 0; i < app->slen; i++) {
            canvas_draw_box(canvas, app->sx[i] * CELL, 12 + app->sy[i] * CELL, CELL, CELL);
        }
        canvas_draw_box(canvas, app->foodx * CELL, 12 + app->foody * CELL, CELL, CELL);
    } else if(app->mode == GamePong) {
        draw_header(canvas, "PONG", app->score);
        canvas_draw_box(canvas, 2, app->paddle_y, 3, 15);
        canvas_draw_disc(canvas, app->ball_x, app->ball_y, 2);
    } else if(app->mode == GameDodge) {
        draw_header(canvas, "DODGE", app->score);
        canvas_draw_box(canvas, app->player_x, 54, 10, 6);
        canvas_draw_box(canvas, app->rock_x, app->rock_y, 7, 7);
    } else {
        draw_header(canvas, "TETRIS", app->score);
        for(int y = 0; y < 13; y++) {
            for(int x = 0; x < 10; x++) {
                if(app->board[y] & (1u << x)) {
                    canvas_draw_box(canvas, 24 + x * 4, 12 + y * 4, 4, 4);
                }
            }
        }
        canvas_draw_box(canvas, 24 + app->tx * 4, 12 + app->ty * 4, 4, 4);
        canvas_draw_box(canvas, 28 + app->tx * 4, 12 + app->ty * 4, 4, 4);
        canvas_draw_box(canvas, 24 + app->tx * 4, 16 + app->ty * 4, 4, 4);
        canvas_draw_box(canvas, 28 + app->tx * 4, 16 + app->ty * 4, 4, 4);
    }
}

static void games_draw_callback(Canvas* canvas, void* context) {
    GamesApp* app = context;

    if(app->mode == GameMenu) {
        draw_menu(canvas, app);
        return;
    }

    uint32_t now = app->ticks;
    uint32_t elapsed = now - app->drawn_ticks;
    if(elapsed > 0) {
        if(elapsed > 4) elapsed = 4;
        for(uint32_t i = 0; i < elapsed; i++) game_update(app);
        app->drawn_ticks = now;
    }

    draw_game(canvas, app);
}

static void games_timer_callback(void* context) {
    GamesApp* app = context;
    app->ticks++;
    view_port_update(app->viewport);
}

static void games_input_callback(InputEvent* event, void* context) {
    GamesApp* app = context;

    if(event->type != InputTypePress && event->type != InputTypeRepeat) return;

    if(app->mode == GameMenu) {
        if(event->key == InputKeyUp) {
            if(app->menu_index > 0) app->menu_index--;
            view_port_update(app->viewport);
        } else if(event->key == InputKeyDown) {
            if(app->menu_index + 1 < GAMES_COUNT) app->menu_index++;
            view_port_update(app->viewport);
        } else if(event->key == InputKeyOk) {
            app->mode = (GameMode)(GameSnake + app->menu_index);
            game_reset(app);
            view_port_update(app->viewport);
        } else if(event->key == InputKeyBack) {
            app->exit_requested = true;
            furi_semaphore_release(app->exit_sem);
        }
        return;
    }

    if(event->key == InputKeyBack) {
        app->mode = GameMenu;
        app->drawn_ticks = app->ticks;
        view_port_update(app->viewport);
        return;
    }

    if(app->mode == GameSnake) {
        if(event->key == InputKeyUp && app->sdy == 0) {
            app->sdx = 0;
            app->sdy = -1;
        } else if(event->key == InputKeyDown && app->sdy == 0) {
            app->sdx = 0;
            app->sdy = 1;
        } else if(event->key == InputKeyLeft && app->sdx == 0) {
            app->sdx = -1;
            app->sdy = 0;
        } else if(event->key == InputKeyRight && app->sdx == 0) {
            app->sdx = 1;
            app->sdy = 0;
        }
    } else if(app->mode == GamePong) {
        if(event->key == InputKeyUp) app->paddle_y -= 4;
        if(event->key == InputKeyDown) app->paddle_y += 4;
        if(app->paddle_y < 12) app->paddle_y = 12;
        if(app->paddle_y > 48) app->paddle_y = 48;
    } else if(app->mode == GameDodge) {
        if(event->key == InputKeyLeft) app->player_x -= 5;
        if(event->key == InputKeyRight) app->player_x += 5;
        if(app->player_x < 2) app->player_x = 2;
        if(app->player_x > 116) app->player_x = 116;
    } else if(app->mode == GameTetris) {
        if(event->key == InputKeyLeft && app->tx > 0) app->tx--;
        if(event->key == InputKeyRight && app->tx < 8) app->tx++;
        if(event->key == InputKeyDown) tetris_step(app);
    }

    view_port_update(app->viewport);
}

int32_t games_app(void* p) {
    UNUSED(p);

    GamesApp app = {0};
    app.mode = GameMenu;
    app.gui = furi_record_open(RECORD_GUI);
    app.viewport = view_port_alloc();
    app.exit_sem = furi_semaphore_alloc(1, 0);

    view_port_draw_callback_set(app.viewport, games_draw_callback, &app);
    view_port_input_callback_set(app.viewport, games_input_callback, &app);
    gui_add_view_port(app.gui, app.viewport, GuiLayerFullscreen);

    app.timer = furi_timer_alloc(games_timer_callback, FuriTimerTypePeriodic, &app);
    furi_timer_start(app.timer, furi_ms_to_ticks(50));

    view_port_update(app.viewport);
    furi_semaphore_acquire(app.exit_sem, FuriWaitForever);

    furi_timer_stop(app.timer);
    furi_timer_free(app.timer);
    gui_remove_view_port(app.gui, app.viewport);
    view_port_free(app.viewport);
    furi_semaphore_free(app.exit_sem);
    furi_record_close(RECORD_GUI);

    return 0;
}
