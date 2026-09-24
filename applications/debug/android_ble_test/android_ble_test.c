#include <furi.h>
#include <furi_hal_bt.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>

#define TAG "AndroidBleTest"

typedef struct {
    Gui* gui;
    ViewDispatcher* dispatcher;
    Submenu* submenu;
    bool active;
} AndroidBleTestApp;

static bool android_ble_test_start(AndroidBleTestApp* app) {
    const uint8_t adv_data[] = {
        0x02, 0x01, 0x06,
        0x0E, 0x09,
        'E','S','P','3','2','-','S','3',' ','T','E','S','T'
    };

    GapExtraBeaconConfig config = {
        .min_adv_interval_ms = 100,
        .max_adv_interval_ms = 100,
        .adv_channel_map = GapAdvChannelMapAll,
        .adv_power_level = GapAdvPowerLevel_0dBm,
        .address_type = GapAddressTypeRandom,
        .address = {0},
    };

    if(!furi_hal_bt_is_available()) {
        FURI_LOG_E(TAG, "BLE is not available");
        return false;
    }

    if(!furi_hal_bt_extra_beacon_set_config(&config) ||
       !furi_hal_bt_extra_beacon_set_data(adv_data, sizeof(adv_data))) {
        FURI_LOG_E(TAG, "Failed to configure BLE test beacon");
        return false;
    }

    if(!furi_hal_bt_extra_beacon_start()) {
        FURI_LOG_E(TAG, "Failed to start BLE test beacon");
        return false;
    }

    app->active = true;
    FURI_LOG_I(TAG, "BLE test beacon started");
    return true;
}

static void android_ble_test_stop(AndroidBleTestApp* app) {
    if(app->active) {
        furi_hal_bt_extra_beacon_stop();
        app->active = false;
        FURI_LOG_I(TAG, "BLE test beacon stopped");
    }
}

static void android_ble_test_toggle(void* context, uint32_t index) {
    UNUSED(index);
    AndroidBleTestApp* app = context;
    furi_assert(app);

    if(app->active) {
        android_ble_test_stop(app);
    } else {
        android_ble_test_start(app);
    }
}

static uint32_t android_ble_test_exit(void* context) {
    AndroidBleTestApp* app = context;
    if(app) android_ble_test_stop(app);
    return VIEW_NONE;
}

int32_t android_ble_test_app(void* p) {
    UNUSED(p);

    AndroidBleTestApp* app = malloc(sizeof(AndroidBleTestApp));
    app->gui = furi_record_open(RECORD_GUI);
    app->dispatcher = view_dispatcher_alloc();
    app->submenu = submenu_alloc();
    app->active = false;

    view_dispatcher_attach_to_gui(
        app->dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    submenu_set_header(app->submenu, "Android BLE test");
    submenu_add_item(
        app->submenu,
        "Start / Stop BLE test",
        0,
        android_ble_test_toggle,
        app);

    view_set_previous_callback(submenu_get_view(app->submenu), android_ble_test_exit);
    view_dispatcher_add_view(
        app->dispatcher,
        0,
        submenu_get_view(app->submenu));

    view_dispatcher_switch_to_view(app->dispatcher, 0);
    view_dispatcher_run(app->dispatcher);

    android_ble_test_stop(app);
    view_dispatcher_remove_view(app->dispatcher, 0);
    submenu_free(app->submenu);
    view_dispatcher_free(app->dispatcher);
    furi_record_close(RECORD_GUI);
    free(app);

    return 0;
}
