/**
 * @file target_input.c
 * Five-button input driver for ESP32-S3 N16R8 ST7735S board.
 *
 * UP=GPIO9, DOWN=GPIO11, LEFT=GPIO12, RIGHT=GPIO13, OK=GPIO14.
 * All buttons are active-low and use the ESP32 internal pull-ups.
 */
#include "target_input.h"

#include <furi.h>
#include <boards/board.h>
#include <driver/gpio.h>

#define TAG "Input5Button"
#define INPUT_DEBOUNCE_POLLS 3U
#define INPUT_LONG_PRESS_MS 500U
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
} Button;

static Button buttons[] = {
    {(gpio_num_t)BOARD_PIN_BUTTON_UP, InputKeyUp, false, false, 0, 0, 0, false},
    {(gpio_num_t)BOARD_PIN_BUTTON_DOWN, InputKeyDown, false, false, 0, 0, 0, false},
    {(gpio_num_t)BOARD_PIN_BUTTON_LEFT, InputKeyLeft, false, false, 0, 0, 0, false},
    {(gpio_num_t)BOARD_PIN_BUTTON_RIGHT, InputKeyRight, false, false, 0, 0, 0, false},
    {(gpio_num_t)BOARD_PIN_BUTTON_OK, InputKeyOk, false, false, 0, 0, 0, false},
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

static void publish_short(FuriPubSub* pubsub, InputKey key, uint32_t* seq) {
    publish(pubsub, key, InputTypePress, seq);
    publish(pubsub, key, InputTypeShort, seq);
    publish(pubsub, key, InputTypeRelease, seq);
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
    }
    FURI_LOG_I(TAG, "5-button input: UP=9 DOWN=11 LEFT=12 RIGHT=13 OK=14");
}

void target_input_poll(FuriPubSub* pubsub, uint32_t* sequence_counter) {
    const uint32_t now = furi_get_tick();
    const uint32_t long_ticks = furi_ms_to_ticks(INPUT_LONG_PRESS_MS);
    const uint32_t repeat_ticks = furi_ms_to_ticks(INPUT_REPEAT_MS);

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
                    publish(pubsub, b->key, InputTypePress, sequence_counter);
                    publish(pubsub, b->key, InputTypeLong, sequence_counter);
                } else if(b->long_sent && now - b->repeat_at >= repeat_ticks) {
                    b->repeat_at = now;
                    publish(pubsub, b->key, InputTypeRepeat, sequence_counter);
                }
            }
            continue;
        }

        b->stable = b->raw;
        if(b->stable) {
            b->pressed_at = now;
            b->repeat_at = now;
            b->long_sent = false;
        } else if(!b->long_sent) {
            publish_short(pubsub, b->key, sequence_counter);
        } else {
            publish(pubsub, b->key, InputTypeRelease, sequence_counter);
        }
    }
}
