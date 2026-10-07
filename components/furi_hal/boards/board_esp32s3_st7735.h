/**
 * @file board_esp32s3_st7735.h
 * ESP32-S3 N16R8 + ST7735S 1.8" 128x160 + five-button board.
 *
 * LCD SCK=GPIO5, MOSI=GPIO6, DC=GPIO7, RST=GPIO15, CS=GPIO16, BL=GPIO4
 * Buttons UP=GPIO9, DOWN=GPIO11, LEFT=GPIO12, RIGHT=GPIO13, OK=GPIO14
 * microSD: CLK=GPIO5, MOSI=GPIO6, MISO=GPIO17, CS=GPIO18
 *
 * The LCD and SD card share SPI2_HOST. CS lines select the active device.
 * User files (/ext) are stored on the microSD card, not LittleFS.
 */
#pragma once

#define BOARD_NAME        "ESP32-S3 N16R8 ST7735S 1.8"
#define BOARD_ID          "esp32s3_st7735"
#define BOARD_ST7735S     1
#define BOARD_HAS_LITTLEFS 0
#define BOARD_LITTLEFS_BASE_PATH "/littlefs"
#define BOARD_LITTLEFS_PARTITION "littlefs"
#define BOARD_TARGET      "esp32s3"

#define BOARD_PIN_BUTTON_UP       9
#define BOARD_PIN_BUTTON_DOWN    11
#define BOARD_PIN_BUTTON_LEFT    12
#define BOARD_PIN_BUTTON_RIGHT   13
#define BOARD_PIN_BUTTON_OK      14
#define BOARD_PIN_BUTTON_BOOT    BOARD_PIN_BUTTON_OK

#define BOARD_PIN_BATTERY_ADC    UINT16_MAX
#define BOARD_HAS_BATTERY        0
#define FURI_HAL_POWER_VIRTUAL_CAPACITY_MAH (0U)

#define BOARD_PIN_LCD_MOSI       6
#define BOARD_PIN_LCD_SCLK       5
#define BOARD_PIN_LCD_DC         7
#define BOARD_PIN_LCD_CS        16
#define BOARD_PIN_LCD_RST       15
#define BOARD_PIN_LCD_BL         4

#define BOARD_LCD_H_RES          160
#define BOARD_LCD_V_RES          128
#define BOARD_LCD_SPI_HOST       SPI2_HOST
#define BOARD_LCD_SPI_FREQ_HZ    (27 * 1000 * 1000)
#define BOARD_LCD_CMD_BITS       8
#define BOARD_LCD_PARAM_BITS    8
#define BOARD_LCD_SWAP_XY        true
#define BOARD_LCD_MIRROR_X       false
#define BOARD_LCD_MIRROR_Y       false
#define BOARD_LCD_MADCTL          0x60
#define BOARD_LCD_INVERT_COLOR   false
#define BOARD_LCD_GAP_X          0
#define BOARD_LCD_GAP_Y          0
#define BOARD_LCD_BL_ACTIVE_LOW  false
#define BOARD_LCD_COLOR_ORDER_BGR false
#define BOARD_LCD_COLMOD         0x05

#define BOARD_LCD_FG_COLOR       0xA0FD
#define BOARD_LCD_FG_COLOR_RB    0x5F03
#define BOARD_LCD_BG_COLOR       0x0000

/* microSD over SPI2_HOST, sharing CLK/MOSI with the LCD. */
#define BOARD_PIN_SD_SCLK         5
#define BOARD_PIN_SD_MOSI         6
#define BOARD_PIN_SD_MISO        17
#define BOARD_PIN_SD_CS          18
#define BOARD_HAS_SD_CARD         1

#define BOARD_PIN_TOUCH_SCL      UINT16_MAX
#define BOARD_PIN_TOUCH_SDA      UINT16_MAX
#define BOARD_PIN_TOUCH_RST      UINT16_MAX
#define BOARD_PIN_TOUCH_INT      UINT16_MAX
#define BOARD_TOUCH_I2C_ADDR     0x00
#define BOARD_TOUCH_I2C_PORT     I2C_NUM_0
#define BOARD_TOUCH_I2C_FREQ_HZ  0
#define BOARD_TOUCH_I2C_TIMEOUT  0

#define BOARD_PIN_CC1101_SCK     UINT16_MAX
#define BOARD_PIN_CC1101_CSN     UINT16_MAX
#define BOARD_PIN_CC1101_MISO    UINT16_MAX
#define BOARD_PIN_CC1101_MOSI    UINT16_MAX
#define BOARD_PIN_CC1101_GDO0    UINT16_MAX

#define BOARD_PIN_IR_TX           UINT16_MAX
#define BOARD_PIN_IR_RX           UINT16_MAX

#define BOARD_PIN_NFC_SCL         UINT16_MAX
#define BOARD_PIN_NFC_SDA         UINT16_MAX
#define BOARD_PIN_NFC_IRQ         UINT16_MAX
#define BOARD_PIN_NFC_RST         UINT16_MAX
#define BOARD_NFC_I2C_PORT        I2C_NUM_0

#define BOARD_PIN_RFID_RX         UINT16_MAX
#define BOARD_PIN_RFID_TX         UINT16_MAX
#define BOARD_RFID_UART_NUM       1

#define BOARD_HAS_NRF24          0
#define BOARD_HAS_TOUCH          0
#define BOARD_HAS_ENCODER       0
#define BOARD_HAS_BLE           1
#define BOARD_HAS_RGB_LED       0
#define BOARD_HAS_VIBRO         0
#define BOARD_HAS_SPEAKER       0
#define BOARD_HAS_IR            0
#define BOARD_HAS_IBUTTON        0
#define BOARD_HAS_RFID          0
#define BOARD_HAS_NFC           0
#define BOARD_HAS_SUBGHZ        0
#define BOARD_HAS_MIC           0
