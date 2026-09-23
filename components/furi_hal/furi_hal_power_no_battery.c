#include "furi_hal_bq27220.h"
#include "furi_hal_bq25896.h"

/* This board has no battery, fuel gauge, or charger IC. These implementations
 * make that hardware absence explicit instead of inventing I2C addresses. */
bool furi_hal_bq27220_init(void) { return false; }
bool furi_hal_bq27220_is_present(void) { return false; }
uint8_t furi_hal_bq27220_get_charge_pct(void) { return 0; }
uint8_t furi_hal_bq27220_get_health_pct(void) { return 0; }
bool furi_hal_bq27220_is_charging(void) { return false; }
uint16_t furi_hal_bq27220_get_voltage_mv(void) { return 0; }
int16_t furi_hal_bq27220_get_current_ma(void) { return 0; }
uint16_t furi_hal_bq27220_get_temperature_raw(void) { return 0; }
uint16_t furi_hal_bq27220_get_remaining_capacity_mah(void) { return 0; }
uint16_t furi_hal_bq27220_get_full_charge_capacity_mah(void) { return 0; }
uint16_t furi_hal_bq27220_get_design_capacity_mah(void) { return 0; }

bool furi_hal_bq25896_init(void) { return false; }
bool furi_hal_bq25896_is_present(void) { return false; }
bool furi_hal_bq25896_is_charging(void) { return false; }
bool furi_hal_bq25896_is_charging_done(void) { return false; }
void furi_hal_bq25896_enable_charging(void) {}
void furi_hal_bq25896_disable_charging(void) {}
uint16_t furi_hal_bq25896_get_vbus_voltage_mv(void) { return 0; }
uint16_t furi_hal_bq25896_get_vbat_voltage_mv(void) { return 0; }
uint16_t furi_hal_bq25896_get_vbat_current_ma(void) { return 0; }
int32_t furi_hal_bq25896_get_temperature_mc(void) { return 0; }
uint16_t furi_hal_bq25896_get_vreg_voltage_mv(void) { return 0; }
void furi_hal_bq25896_set_vreg_voltage_mv(uint16_t vreg_mv) { (void)vreg_mv; }
void furi_hal_bq25896_enable_otg(void) {}
void furi_hal_bq25896_disable_otg(void) {}
bool furi_hal_bq25896_is_otg_enabled(void) { return false; }
bool furi_hal_bq25896_is_vbus_present(void) { return false; }
void furi_hal_bq25896_poweroff(void) {}
