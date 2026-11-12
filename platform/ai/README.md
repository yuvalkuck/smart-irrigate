# AI Platform

Firmware project for the **companion AI module**, an ESP32-S3 DevKitC-1. It is a separate ESP-IDF
project (its own CLion project, its own `CMakeLists.txt`, `sdkconfig` and `partitions.csv`), not a
component of the ESP32-C6 irrigation firmware in the repository root (`../../`).

## Role

The ESP32-C6 irrigation computer streams the microclimate and water-supply data to this module. The
module runs local AI/ML inference on it and adapts the irrigation configuration to observed and
forecast weather conditions.

* Inference always runs standalone on the ESP32-S3. It does not need the central server.
* The model is trained offline on an external server, from telemetry collected by the station plus free
  historical data from existing meteorological services. The trained model is pushed down to the
  ESP32-S3.
* The module also drives the LCD2004 display.

## Power

| State | S3 |
|---|---|
| Mains on | On |
| Battery on | **Off** (with Wi-Fi) |

It is fed 5 V from the main box buck converter. It is a mains-only feature, so the LCD is off with it.

## Link to the ESP32-C6

UART between the two chips:

| C6 | S3 |
|---|---|
| GPIO 6 (TX) | RX pin |
| GPIO 7 (RX) | TX pin |

The S3 pin numbers for this UART are **TBD**. The wire protocol is **TBD**. Keep the payload layout
independent of the transport, as the C6 `protocol` component does (see `../../doc/stractures.md`).

## LCD2004

I2C backpack on the S3's own I2C bus, not the C6 sensor bus. Proposed S3 pins, to be confirmed:

| Function | S3 GPIO |
|---|---|
| I2C SDA | 8 |
| I2C SCL | 9 |
| Wake button | 4 |
| LCD power switch | 5 |

The display wakes on the button and switches off after N minutes. The pins avoid the S3 strapping pins
0/3/45/46, the USB pins 19/20 and the octal-PSRAM pins 35-37 on N16R8 boards.

## Project setup

* Target: `esp32s3`, ESP-IDF v6.1, C++20, exceptions and RTTI off (same conventions as the C6 project).
* Create the project in CLion with its own ESP-IDF toolchain profile:

```bash
. ~/esp/esp-idf/export.sh
idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash monitor
```

The CLion project is not created yet; this directory only holds the design notes.

## Open items

* S3 pins for the UART to the C6.
* C6 to S3 data protocol.
* Model format, size and update path (flash partition for the model, push from the server).
* Which weather data and telemetry fields the model takes as input.
