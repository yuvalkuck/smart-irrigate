# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Smart Irrigate is an ESP-IDF (v6.1) firmware project for an **ESP32-C6** (FireBeetle 2) irrigation
controller. It monitors ambient/soil sensors, computes irrigation demand from macro-environmental
data (rather than local soil-moisture probes), and drives up to 6 relay-controlled water valves. It
talks to the outside world via MQTT (telemetry out / commands in) and is provisioned over a
temporary SoftAP + HTTP server. See `README.md` (and the split design docs in `doc/`: power, placement, progress plan, build, configuration) for the full design rationale (evapotranspiration
model, hydraulic zero-pressure handling, valve scheduling) and `doc/stractures.md` for the
currently-implemented wire protocol design.

## Build

This is a standard ESP-IDF project (`CMakeLists.txt` at repo root calls
`$ENV{IDF_PATH}/tools/cmake/project.cmake`). Source the export script of your ESP-IDF install before
building (on the main dev machine `~/esp/active` is a symlink to `~/esp/v6.1`):

```bash
. ~/esp/active/esp-idf/export.sh
idf.py set-target esp32c6      # target is also force-set in CMakeLists.txt
idf.py build
idf.py -p <PORT> flash monitor
```

Target is pinned to `esp32c6` in the root `CMakeLists.txt` (`IDF_TARGET` cache var), so this is not a
multi-target project.

### First-boot provisioning defaults (optional)

`components/common/gen/default_initiate.in.h` is templated at configure time from environment
variables (`INITIATE_WIFI_SSID`, `INITIATE_WIFI_PASSWORD`, `INITIATE_DEVICE_UNIQUE`,
`INITIATE_BROKER_URL`, `INITIATE_MQTT_CLIENT_USERNAME`/`PASSWORD`, `INITIATE_NTP_SERVER`,
`INITIATE_DEFAULT_LOCALE_TIME_ZONE`) into `components/common/include/default_initiate.h`. Set these
in the shell before running `idf.py build`/`cmake` if you need non-empty defaults baked in; otherwise
they configure_file to empty strings and provisioning happens entirely at runtime via the SoftAP flow.

### Device configuration (Kconfig)

Run `idf.py menuconfig` to adjust project-specific settings defined in `main/Kconfig.projbuild`:
valve count (`DEVICE_MAX_VALVES`, must not exceed the `gpio_valve_<n>` entries in
`components/board_manager/board_peripherals.yaml`), max programs per valve (`DEVICE_MAX_VALVE_CONFIG`), LCD
presence (`DEVICE_LCD_ENABLE`), power-state valve limits (`DEVICE_MAX_SIMULTANEOUS_OPEN_VALVES` under
"Main Power ON", `DEVICE_MAX_ROUND_ROBIN_TIME_MINI` slice minutes under "Main Battery ON"), and SoftAP
provisioning defaults (password, gateway/mask, SSID suffix). Valve GPIOs are declared in `components/board_manager/board_peripherals.yaml`
(ESP Board Manager, `idf.py bmgr -b board_manager`), not in Kconfig. Changing them is a firmware
rebuild, not a runtime option — pins are compiled in.

The mains/battery options are defined in Kconfig but no firmware code reads them yet — the
power-path behaviour is a design in `doc/PowerArchitecture.md` and `doc/PhysicalPlacement.md`
(24 V DC bus, battery backup, one valve at a time on battery with Wi-Fi and the ESP32-S3
off), not implemented. Treat those docs as design intent, not current behaviour.

Pin assignments live in `components/board_manager/board_peripherals.yaml` (ESP Board Manager: status LED,
config-mode boot switch, 1-Wire pin, RS-485 pins, I2C SDA/SCL, pressure ADC channel, valves); code reads them
from the board manager handles/configs. Non-pin board constants (RS-485 UART port, wind ADC channel) come from
`hardware/boards/<BOARD>.cmake`, generated into `components/common/include/gpio_declaraion.h` at configure time — cross-check against the wiring
table in the "Hardware Pin Configurations" section of `doc/ProjectConfiguration.md` before touching hardware, since it
documents a pin *rewire* that supersedes the original table (e.g. config switch moved to GPIO 23,
DS18B20 to GPIO 4).

## Host-side test tooling (not firmware)

`test/` and `utest/` are separate host-native CMake projects, not part of the ESP-IDF build (details
in the `CLAUDE.md` of each directory).

The `certs` symlink at repo root points at `test/mosquitto/config/certs` and is embedded into the
firmware image via `EMBED_TXTFILES` in `components/hmqtt/CMakeLists.txt` (`ca.crt`, `client.crt`,
`client.key`) — these must exist before building firmware that includes `hmqtt`.

There is no on-device unit test framework in this repo; correctness is validated by building,
flashing, and observing serial logs / MQTT traffic.

## Architecture

### Boot flow (`main/entrypoint.cpp`)

`app_main()` always initializes GPIO (valve relays, LED, 1-Wire bus) and ADC (water pressure, wind
speed channels) first, then branches on `CONFIG_MODE_Pin` (GPIO 23) read at boot **and** whether
Wi-Fi credentials exist in NVS:

- **Configuration Mode** (button held low at boot, or no stored Wi-Fi SSID): starts a SoftAP + HTTP
  server (`components/hwifi/soft_ap.cpp`, `ap_webserver.cpp`, built on ESP-IDF's `esp_http_server`)
  so a companion app can push Wi-Fi/MQTT credentials into the `setup` NVS partition. No sensors/MQTT
  are started in this mode.
- **Operational Mode**: registers the app event handler, brings up telemetry, connects Wi-Fi STA,
  starts SNTP (MQTT is only started from the SNTP time-sync callback,
  `continue_after_time_sync_cb`, so the device has a valid clock before it publishes/subscribes),
  and starts the telemetry sampling task.

### Eventing model

Cross-component communication (sensor task → MQTT publish, MQTT command → config apply) goes through
one esp_event loop/base (`COMMON_BASE_EVENTS`, declared in `common/include/common_event.h`,
registered in `main/event_handler.cpp`), not direct component-to-component calls. When adding a new
async data source or command, prefer posting/handling an event on this bus over adding new direct
dependencies between components.

### Storage partitions (`partitions.csv`)

Two custom data partitions beyond the standard `nvs`/`phy_init`/`factory`: `setup` (small NVS blob —
Wi-Fi/MQTT/NTP credentials, written only during Configuration Mode) and `config` (raw, non-NVS
partition holding the binary `Configuration`/`Valve`/`Program`/`Task` structures from `protocol`, read
via partition mmap — see `UnmapperPartitionDMA` in `base_command.h`).

* **Storage factory is HUGE for Debug state.**  

## Conventions

- C++20 (`-std=gnu++20`) with exceptions and RTTI disabled (`-fno-exceptions -fno-rtti`) across every
  component — don't introduce `try`/`catch`, `dynamic_cast`, or code that assumes them.
- ESP-IDF component boundaries are enforced through `REQUIRES`/`PRIV_REQUIRES` in each
  `CMakeLists.txt`; when a `.cpp` needs a new component's headers, add it there rather than relying
  on include-path leakage.
- Debug-only instrumentation (`METHODTRACE`, `LOGTRACE`) compiles out entirely in release builds;
  don't rely on it for control flow.
