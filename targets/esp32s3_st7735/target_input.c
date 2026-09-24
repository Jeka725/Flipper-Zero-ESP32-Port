/**
 * @file target_input.c
 * Five-button input driver for ESP32-S3 N16R8 ST7735S board.
 *
 * Exact physical mapping: SELECT=GPIO14, RIGHT=GPIO13, LEFT=GPIO12,
 * UP=GPIO11, DOWN=GPIO9.
 * All buttons are active-low and use the ESP32 internal pull-ups.
 */
#include "target_input.h"

#include <furi.h>
#include <boards/board.h>
#include <driver/gpio.h>

#define TAG "Input5Button"
#define INPUT_DEBOUNCE_POLLS 3U
#define INPUT_LONG_PRESS_MS 2000U
#define INPUT_REPEAT_MS 200U

typedef struct {
    gpio_num_t pin;
    InputKey key;
    bool raw;
    bool stable;
    uint8_t debounce;
    uint32_t pressed_at;
    uint32_t repeat_at;
    bool long_sent;
    bool back_on_long;
} Button;

static Button buttons[] = {
    {(gpio_num_t)BOARD_PIN_BUTTON_UP, InputKeyUp, false, false, 0, 0, 0, false, false},
    {(gpio_num_t)BOARD_PIN_BUTTON_DOWN, InputKeyDown, false, false, 0, 0, 0, false, false},
    {(gpio_num_t)BOARD_PIN_BUTTON_LEFT, InputKeyLeft, false, false, 0, 0, 0, false, false},
    {(gpio_num_t)BOARD_PIN_BUTTON_RIGHT, InputKeyRight, false, false, 0, 0, 0, false, false},
    {(gpio_num_t)BOARD_PIN_BUTTON_OK, InputKeyOk, false, false, 0, 0, 0, false, false},
};

static bool combo_back_active = false;
static uint32_t combo_back_started = 0;
static bool combo_back_sent = false;

static void publish(FuriPubSub* pubsub, InputKey key, InputType type, uint32_t* seq) {
    InputEvent event = {
        .sequence_source = INPUT_SEQUENCE_SOURCE_HARDWARE,
        .sequence_counter = ++(*seq),
        .key = key,
        .type = type,
    };
    furi_pubsub_publish(pubsub, &event);
}

static bool pressed(const Button* b) {
    return gpio_get_level(b->pin) == 0;
}

void target_input_init(void) {
    for(size_t i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) {
        gpio_config_t cfg = {
            .pin_bit_mask = 1ULL << buttons[i].pin,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&cfg));
        buttons[i].raw = pressed(&buttons[i]);
        buttons[i].stable = buttons[i].raw;
        buttons[i].debounce = INPUT_DEBOUNCE_POLLS;
        buttons[i].back_on_long = false;
    }
    FURI_LOG_I(TAG, "5-button input: SELECT=14 RIGHT=13 LEFT=12 UP=11 DOWN=9; UP+DOWN hold=2s Back");
}

void target_input_poll(FuriPubSub* pubsub, uint32_t* sequence_counter) {
    const uint32_t now = furi_get_tick();
    const uint32_t long_ticks = furi_ms_to_ticks(INPUT_LONG_PRESS_MS);
    const uint32_t repeat_ticks = furi_ms_to_ticks(INPUT_REPEAT_MS);

    /* UP + DOWN held together for 2 seconds is a dedicated Back shortcut.
     * Send a normal Back Press/Release pair (not InputTypeLong): Back navigation
     * in Flipper applications is handled by the ordinary Press event. */
    const bool combo_now = (gpio_get_level((gpio_num_t)BOARD_PIN_BUTTON_UP) == 0) &&
                           (gpio_get_level((gpio_num_t)BOARD_PIN_BUTTON_DOWN) == 0);
    if(combo_now && !combo_back_active) {
        combo_back_active = true;
        combo_back_started = now;
        combo_back_sent = false;
    } else if(!combo_now) {
        if(combo_back_sent) {
            publish(pubsub, InputKeyBack, InputTypeRelease, sequence_counter);
        }
        combo_back_active = false;
        combo_back_sent = false;
    } else if(!combo_back_sent && now - combo_back_started >= long_ticks) {
        combo_back_sent = true;
        publish(pubsub, InputKeyBack, InputTypePress, sequence_counter);
    }

    for(size_t i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) {
        Button* b = &buttons[i];
        const bool raw = pressed(b);

        if(raw != b->raw) {
            b->raw = raw;
            b->debounce = 1;
            continue;
        }
        if(b->debounce < INPUT_DEBOUNCE_POLLS) {
            b->debounce++;
            continue;
        }
        if(b->stable == b->raw) {
            if(b->stable) {
                const uint32_t held = now - b->pressed_at;
                if(!b->long_sent && held >= long_ticks) {
                    b->long_sent = true;
                    b->repeat_at = now;

                    /*
                     * ViewDispatcher requires a matching Press before Long.
                     * A synthetic Back press/long pair makes OK-hold work as
                     * navigation Back without turning a normal OK press into Back.
                     */
                    if(b->key == InputKeyOk) {
                        publish(pubsub, b->key, InputTypeLong, sequence_counter);
                    } else {
                        publish(pubsub, b->key, InputTypeLong, sequence_counter);
                    }
                } else if(b->long_sent && now - b->repeat_at >= repeat_ticks) {
                    b->repeat_at = now;
                    if(b->key != InputKeyOk) {
                        publish(pubsub, b->key, InputTypeRepeat, sequence_counter);
                    }
                }
            }
            continue;
        }

        b->stable = b->raw;
        if(b->stable) {
            b->pressed_at = now;
            b->repeat_at = now;
            b->long_sent = false;
            b->back_on_long = false;
            publish(pubsub, b->key, InputTypePress, sequence_counter);
        } else {
            /* Short MUST be sent before Release. */
            if(!b->long_sent) {
                publish(pubsub, b->key, InputTypeShort, sequence_counter);
            }
            publish(pubsub, b->key, InputTypeRelease, sequence_counter);
        }
    }
}
