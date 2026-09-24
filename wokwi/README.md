# Wokwi ST7735S runtime test

Ready-to-run Wokwi project for ESP32-S3 N16R8 + 1.8-inch ST7735S.

## Pins

| ST7735S | ESP32-S3 |
|---|---:|
| SCK | GPIO5 |
| MOSI | GPIO6 |
| DC/A0 | GPIO7 |
| BL | GPIO4 (not simulated by the custom chip) |
| RST | GPIO15 |
| CS | GPIO16 |
| VCC | 3V3 |
| GND | GND |

The Wokwi ST7735 custom chip exposes SCK, MOSI, CS, DS, RST, VCC and GND. DS is the display data/command line. GPIO4 is intentionally not connected because this custom chip has no backlight pin.

## Firmware

Use the merged esp32s3_st7735.bin from the GitHub Actions artifact flipper-esp32s3-st7735-firmware.

The merged image already contains the bootloader, partition table, application and LittleFS image at 0xC20000. Do not upload littlefs.bin separately.

## Browser steps

1. Open Wokwi and create/open an ESP32-S3 project.
2. Copy the diagram.json from this folder into the Wokwi project.
3. Download the GitHub Actions artifact flipper-esp32s3-st7735-firmware and extract esp32s3_st7735.bin.
4. In Wokwi choose F1 -> Upload Firmware and Start Simulation.
5. Select esp32s3_st7735.bin.
6. Open Serial Monitor and start the simulation.
7. Send me a screenshot of the display while the Dolphin animation is running and the complete Serial Monitor output from boot.

Wokwi supports custom ESP32 application firmware, including ESP-IDF, and the browser uploader accepts a complete ESP32 .bin image.

## Proof required

For animation proof, the screenshot should show the display and the serial log should contain the LittleFS mount, manifest lookup, animation selection, frame loading, and changing frame numbers.

Wokwi cannot prove BLE runtime for this project because Bluetooth is not simulated; BLE runtime needs the physical ESP32-S3 hardware.
