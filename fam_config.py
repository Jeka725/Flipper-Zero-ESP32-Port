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
    "games",

    # Original games already present in this port.
    "snake",
    "tetris",
    "flipper_pong",
    "game15",
    "snake20",

    "about",
    "bt_settings",
    "example_apps_data",
    "example_apps_assets",
    "example_number_input",
    "clock",
    "bad_usb",
    "subghz",
    "cli_subghz",
    "subghz_load_dangerous_settings",
    "passport",
    "nfc",
    "infrared",
    "lfrfid",
    "wlan",
    "wifi",
    "ota_updater",
    "streaming",
    "nrf24",
    "ble_spam",
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
]

_board = os.environ.get("FLIPPER_BOARD", "")
_boards_without_nfc = {"waveshare_c6", "waveshare_c6_1.9", "waveshare_c6_1.47", "esp32s3_st7735"}
_boards_without_ir = {"waveshare_c6", "waveshare_c6_1.9", "waveshare_c6_1.47", "esp32s3_st7735"}

# Original Doom/Wolf3D remain in the source tree but are not enabled for the
# 128x160 ST7735S target because their original renderers require the larger
# display/audio configuration.
_boards_without_wolf3d = {"waveshare_c6", "waveshare_c6_1.9", "waveshare_c6_1.47", "esp32s3_st7735"}

if _board in _boards_without_nfc:
    APPS = [a for a in APPS if a != "nfc"]

_boards_without_subghz = {"waveshare_c6_1.47", "esp32s3_st7735"}

if _board in _boards_without_ir:
    APPS = [a for a in APPS if a not in ("infrared", "js_infrared")]

if _board in _boards_without_subghz:
    APPS = [a for a in APPS if a not in ("subghz", "cli_subghz", "subghz_load_dangerous_settings", "js_subghz")]

_boards_without_nrf24 = {"waveshare_c6", "waveshare_c6_1.9", "waveshare_c6_1.47", "esp32s3_st7735"}

if _board in _boards_without_nrf24:
    APPS = [a for a in APPS if a != "nrf24"]

# The ST7735S build keeps the original core services, Wi-Fi/BLE and the
# original games, while dropping optional payloads not used on this hardware.
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
        "js_blebeacon",
    }
    APPS = [a for a in APPS if a not in _optional_st7735_apps]

# cli_vcp and dolphin stay because desktop depends on them for qFlipper /
# USB-Storage integration. TinyUSB composite is installed lazily so the
# USB-Serial-JTAG bridge remains available for esptool.

EXTRA_EXT_APPS = []
TARGET_HW = 32
AUTORUN_APP = ""
