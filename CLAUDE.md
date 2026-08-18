# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Smart Irrigate is an ESP-IDF (v6.0) firmware project for an **ESP32-C6** (FireBeetle 2) irrigation
controller. It monitors ambient/soil sensors, computes irrigation demand from macro-environmental
data (rather than local soil-moisture probes), and drives up to 6 relay-controlled water valves. It
talks to the outside world via MQTT (telemetry out / commands in) and is provisioned over a
temporary SoftAP + raw TCP server. See `README.md` for the full design rationale (evapotranspiration
model, hydraulic zero-pressure handling, valve scheduling) and `doc/stractures.md` / `doc/FUTURE.md`
for the wire protocol design (current vs. planned).

## Build

This is a standard ESP-IDF project (`CMakeLists.txt` at repo root calls
`$ENV{IDF_PATH}/tools/cmake/project.cmake`). ESP-IDF is expected at `~/esp/esp-idf`; source its
export script before building:

```bash
. ~/esp/esp-idf/export.sh
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
valve count (`DEVICE_MAX_VALVES`), the valve GPIO list (`DYNAMIC_VALVE_GPIO_LIST`, comma-separated,
parsed at boot in `main/entrypoint.cpp`), max programs per valve, and SoftAP provisioning defaults
(password, gateway/mask, SSID suffix). Changing valve GPIOs is a firmware rebuild, not a runtime
option — pins are compiled in.

Fixed (non-Kconfig) pin assignments live in `components/common/include/gpio_declaraion.h`
(status LED, config-mode boot switch, 1-Wire bus, I2C SDA/SCL) — cross-check against the wiring
tables in `README.md` §4.1 before touching hardware, since the README documents a pin *rewire* that
supersedes the original table (e.g. config switch moved to GPIO 23, DS18B20 to GPIO 4).

## Host-side test tooling (not firmware)

`test/genpaylaod/` is a **separate, host-native CMake project** (not part of the ESP-IDF build) that
compiles a small CLI11-based tool against `components/protocol/include/*` with `-DTESTER` to
generate/inspect binary protocol payloads without a device. Build it standalone:

```bash
cmake -S test/genpaylaod -B test/genpaylaod/build
cmake --build test/genpaylaod/build
```

Protocol headers (`base_command.h`, `configuration.h`) `#if !defined(TESTER)` out ESP-IDF-only types
(`esp_partition.h`, `std::unique_ptr` partition wrapper) so they compile in this host context — keep
new protocol code TESTER-safe if it needs to be exercised from this tool.

`test/mosquitto/` and `components/hmqtt/test/mosquitto/` hold docker-compose'd Mosquitto brokers
(with TLS certs) for exercising MQTT locally; `test/mqtt_brocker.py` is an alternative pure-Python
(`amqtt`) broker for the same purpose (deps in `test/requires.txt`). `test/mosquitto/publish.sh` /
`monitor.sh` publish/subscribe via `docker exec` (`dexec` alias expected) against topics like
`/client/configuration`. The `certs` symlink at repo root points at
`test/mosquitto/config/certs` and is embedded into the firmware image via
`EMBED_TXTFILES` in `components/hmqtt/CMakeLists.txt` (`ca.crt`, `client.crt`, `client.key`) — these
must exist before building firmware that includes `hmqtt`.

There is no on-device unit test framework in this repo; correctness is validated by building,
flashing, and observing serial logs / MQTT traffic.

## Architecture

### Boot flow (`main/entrypoint.cpp`)

`app_main()` always initializes GPIO (valve relays, LED, 1-Wire bus) and ADC (water pressure, wind
speed channels) first, then branches on `GPIO_CONFIG_MODE_PIN` (GPIO 23) read at boot **and** whether
Wi-Fi credentials exist in NVS:

- **Configuration Mode** (button held low at boot, or no stored Wi-Fi SSID): starts a SoftAP +
  webserver/TCP listener (`components/hwifi/soft_ap.cpp`, `ap_webserver.cpp`) so a companion app can
  push Wi-Fi/MQTT credentials into the `setup` NVS partition. No sensors/MQTT are started in this
  mode.
- **Operational Mode**: registers the app event handler, brings up telemetry, connects Wi-Fi STA,
  starts SNTP (MQTT is only started from the SNTP time-sync callback,
  `continue_after_time_sync_cb`, so the device has a valid clock before it publishes/subscribes),
  and starts the telemetry sampling task.

### Component layout (`components/`)

Each component is a standalone ESP-IDF component with its own `CMakeLists.txt`; dependency direction
generally flows `main → {flash, hwifi, hmqtt, hsntp, telemetry, protocol} → common`:

- **`common`** — shared, header-only-ish: `gpio_declaraion.h` (pin map), `common_event.h`
  (`COMMON_BASE_EVENTS` esp_event bus + `EventData`/`TelemetryData` structs), `logger.h`
  (`METHODTRACE`/`LOGTRACE` macros, compiled to no-ops unless `_DEBUG` is defined via
  `CMAKE_BUILD_TYPE=Debug`), generated `default_initiate.h`.
- **`flash`** — NVS wrapper (`NvsConfig` RAII class) plus the `CFG_NVS_KEY_*` string constants for
  Wi-Fi/MQTT/NTP/timezone/device-unique-id storage in the `setup` partition.
- **`hwifi`** — Wi-Fi STA (`hwifi_sta.cpp`) and SoftAP + provisioning webserver
  (`soft_ap.cpp`, `ap_webserver.cpp`) — mutually exclusive modes selected in `entrypoint.cpp`.
- **`hmqtt`** — MQTT client over TLS (certs embedded from `certs/`); `mqtt_publish(topic, payload)`
  is the only outbound API; inbound commands land back on the app via `COMMON_BASE_EVENTS`.
- **`hsntp`** — SNTP client; `init_sntp(server, cb)` fires `cb` once time sync completes.
- **`telemetry`** — owns the shared I2C master bus, ADC unit, and 1-Wire bus handles, and drives all
  sensors from one periodic FreeRTOS task (`cbTelemetryTask`, 10s interval) that posts
  `COMMON_EVENT_UPDATED_SENSOR` on `COMMON_BASE_EVENTS`. Sensor drivers live under
  `telemetry/inc/sensor_*.h` + `telemetry/src/sensor_*.cpp`, each implementing one of the
  `SensorConcept`/`SensorI2C`/`SensorADC`/`SensorOneWire` C++20 concepts declared in
  `telemetry/inc/sensor_concept.h` (compile-time-enforced via `AbstractSensorI2C<Derived>` etc. —
  every sensor needs `init(bus)`, `online(bus)`, and `read(TelemetryData&)`). New sensors should
  follow this pattern rather than being wired ad hoc into `telemetry.cpp`.
- **`protocol`** — binary MQTT command/configuration payload (de)serialization
  (`getPayloadCommand`, `setConfiguration`, `getConfiguration`). Structs are `#pragma pack`ed and
  guarded with `#if !defined(TESTER)` so the same headers compile both on-device and in the host
  `test/genpaylaod` tool. **The wire format here is actively evolving** — `doc/stractures.md`
  documents the currently-implemented `Command`/`Configuration` layout; `doc/FUTURE.md` is a design
  scratchpad for the next revision (start/stop commands, repeat rules, environment-conditioned
  triggers) and is not yet implemented — don't treat it as current behavior.
- **`bmp5api`** — vendored Bosch BMP5xx sensor API (upstream driver, avoid modifying — see its own
  `README.md`/`LICENSE` under `components/bmp5api/1.5.0/`).

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
