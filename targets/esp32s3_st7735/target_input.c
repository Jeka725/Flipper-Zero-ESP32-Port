/**
 * @file target_input.c
 * Five-button input driver for ESP32-S3 N16R8 ST7735S board.
 *
 * Exact physical mapping:
 *   SELECT=GPIO14, RIGHT=GPIO13, LEFT=GPIO12, UP=GPIO9, DOWN=GPIO11.
 * All buttons are active-low and use the ESP32 internal pull-ups.
 *
 * Back shortcut:
 *   Holding ANY of the five buttons for 2 seconds generates
 *   InputKeyBack/InputTypeShort immediately, before the button is released.
 */
#include "target_input.h"

#include <furi.h>
#include <boards/board.h>
#include <driver/gpio.h>

#define TAG "Input5Button"
#define INPUT_DEBOUNCE_POLLS 3U
#define INPUT_BACK_HOLD_MS 2000U

typedef struct {
    gpio_num_t pin;
    InputKey key;
    bool raw;
    bool stable;
    uint8_t debounce;
    uint32_t pressed_at;
    bool back_sent;
} Button;

static Button buttons[] = {
    {(gpio_num_t)BOARD_PIN_BUTTON_UP, InputKeyUp, false, false, 0, 0, false},
    {(gpio_num_t)BOARD_PIN_BUTTON_DOWN, InputKeyDown, false, false, 0, 0, false},
    {(gpio_num_t)BOARD_PIN_BUTTON_LEFT, InputKeyLeft, false, false, 0, 0, false},
    {(gpio_num_t)BOARD_PIN_BUTTON_RIGHT, InputKeyRight, false, false, 0, 0, false},
    {(gpio_num_t)BOARD_PIN_BUTTON_OK, InputKeyOk, false, false, 0, 0, false},
};

static void publish(FuriPubSub* pubsub, InputKey key, InputType type, uint32_t* seq) {
    InputEvent event = {
        .sequence_source = INPUT_SEQUENCE_SOURCE_HARDWARE,
        .sequence_counter = ++(*seq),
        .key = key,
        .type = type,
    };
    furi_pubsub_publish(pubsub, &event);
}

static bool button_pressed(const Button* b) {
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

        buttons[i].raw = button_pressed(&buttons[i]);
        buttons[i].stable = buttons[i].raw;
        buttons[i].debounce = INPUT_DEBOUNCE_POLLS;
        buttons[i].pressed_at = 0;
        buttons[i].back_sent = false;
    }

    FURI_LOG_I(
        TAG,
        "Input: UP=9 DOWN=11 LEFT=12 RIGHT=13 SELECT=14; any button held 2s = Back");
}

void target_input_poll(FuriPubSub* pubsub, uint32_t* sequence_counter) {
    const uint32_t now = furi_get_tick();
    const uint32_t back_hold_ticks = furi_ms_to_ticks(INPUT_BACK_HOLD_MS);

    for(size_t i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++) {
        Button* b = &buttons[i];
        const bool raw = button_pressed(b);

        /* Debounce the physical GPIO. */
        if(raw != b->raw) {
            b->raw = raw;
            b->debounce = 1;
            continue;
        }

        if(b->debounce < INPUT_DEBOUNCE_POLLS) {
            b->debounce++;
            continue;
        }

        /* Stable state unchanged: check the 2-second Back timer. */
        if(b->stable == b->raw) {
            if(b->stable && !b->back_sent &&
               (now - b->pressed_at >= back_hold_ticks)) {

                b->back_sent = true;

                /*
                 * Flipper's normal logical Back handling commonly consumes
                 * InputTypeShort. Send it HERE at 2 seconds, not on release,
                 * and never send InputTypePress for the Back shortcut.
                 */
                publish(pubsub, InputKeyBack, InputTypeShort, sequence_counter);
                FURI_LOG_I(TAG, "2s hold: key=%d -> Back Short", (int)b->key);
            }
            continue;
        }

        /* Stable state changed. */
        b->stable = b->raw;

        if(b->stable) {
            /* Physical press. */
            b->pressed_at = now;
            b->back_sent = false;
            publish(pubsub, b->key, InputTypePress, sequence_counter);
        } else {
            /* Physical release. */
            if(!b->back_sent) {
                /* Normal click. */
                publish(pubsub, b->key, InputTypeShort, sequence_counter);
            }

            publish(pubsub, b->key, InputTypeRelease, sequence_counter);
            b->back_sent = false;
        }
    }
}
