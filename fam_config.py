import os

MANIFEST_ROOTS = [
    "components",
    "applications",
    "applications_user",
]

APP_SOURCE_OVERRIDES = {
    "desktop": "applications",
    "storage": "applications",
}

APPS = [
    "input",
    "notification",
    "gui",
    "dialogs",
    "locale",
    "cli",
    "cli_vcp",
    "storage",
    "storage_start",
    "power",
    "power_start",
    "power_settings",
    "loader",
    "loader_start",
    "namechanger_srv",
    "spoofing_settings",
    "notification_settings",
    "interface_settings",
    "desktop",
    "archive",

    # Original games already present in this port. The custom replacement
    # application/games implementation is intentionally not built.
    "snake_game",
    "tetris",
    "pong",
    "game15",
    "snake20",

    "about",
    "bt_settings",
    "clock",
    "passport",
    "wlan",
    "wifi",
    "ota_updater",
    "nfc",
    "infrared",
    "lfrfid",
    "streaming",
    "nrf24",
    "ble_spam",
]

_board = os.environ.get("FLIPPER_BOARD", "")

_boards_without_nfc = {
    "waveshare_c6",
    "waveshare_c6_1.9",
    "waveshare_c6_1.47",
    "esp32s3_st7735",
}
_boards_without_ir = {
    "waveshare_c6",
    "waveshare_c6_1.9",
    "waveshare_c6_1.47",
    "esp32s3_st7735",
}
_boards_without_subghz = {"waveshare_c6_1.47", "esp32s3_st7735"}
_boards_without_nrf24 = {
    "waveshare_c6",
    "waveshare_c6_1.9",
    "waveshare_c6_1.47",
    "esp32s3_st7735",
}
_boards_without_wolf3d = {
    "waveshare_c6",
    "waveshare_c6_1.9",
    "waveshare_c6_1.47",
    "esp32s3_st7735",
}

if _board in _boards_without_nfc:
    APPS = [a for a in APPS if a != "nfc"]

if _board in _boards_without_ir:
    APPS = [a for a in APPS if a != "infrared"]

if _board in _boards_without_subghz:
    APPS = [a for a in APPS if a != "subghz"]

if _board in _boards_without_nrf24:
    APPS = [a for a in APPS if a != "nrf24"]

# Keep the compact ST7735S build focused on the original core UI, wireless
# features and original games. Heavy/unused multimedia and JS payloads are
# intentionally excluded for this 128x160 target.
if _board == "esp32s3_st7735":
    _optional_st7735_apps = {
        "streaming",
        "bad_usb",
        "example_apps_data",
        "example_apps_assets",
        "example_number_input",
        "js_app",
        "js_event_loop",
        "js_gui",
        "js_gui__loading",
        "js_gui__empty_screen",
        "js_gui__submenu",
        "js_gui__text_input",
        "js_gui__number_input",
        "js_gui__button_panel",
        "js_gui__popup",
        "js_gui__button_menu",
        "js_gui__menu",
        "js_gui__vi_list",
        "js_gui__byte_input",
        "js_gui__text_box",
        "js_gui__dialog",
        "js_gui__file_picker",
        "js_gui__widget",
        "js_gui__icon",
        "js_notification",
        "js_math",
        "js_storage",
        "js_subghz",
        "js_infrared",
        "js_blebeacon",
    }
    APPS = [a for a in APPS if a not in _optional_st7735_apps]

EXTRA_EXT_APPS = []
TARGET_HW = 32
AUTORUN_APP = ""
