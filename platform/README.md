# Platform

Everything that runs on a device other than the ESP32-C6 irrigation controller. The controller
firmware itself is the ESP-IDF project in the repository root (`../main`, `../components`).

```
platform/
├── ai/          ESP32-S3 companion AI module (ESP-IDF project)
├── sensor/      STM32C011 sensor cluster (STM32CubeMX project)
└── iconnect/    IrrigateConnect, Android provisioning app (Gradle project)
```

Each directory is its own project with its own build. None of them is part of the root ESP-IDF build.

| Directory | Runs on | Toolchain | Talks to | Details |
|---|---|---|---|---|
| `ai/` | ESP32-S3 DevKitC-1, in the main box | ESP-IDF v6.0, `esp32s3`, C++20 | ESP32-C6 over UART (GPIO 6 and 7 on the C6) | [ai/README.md](ai/README.md) |
| `sensor/` | STM32C011F6P3TR, on the sensor mast | STM32CubeMX 6.18.1, STM32Cube FW_C0 V1.4.1, GCC | ESP32-C6 over RS-485, Modbus RTU slave | [sensor/README.md](sensor/README.md) |
| `iconnect/` | Android phone (minSdk 30), Kotlin and Jetpack Compose | Gradle, Android Studio | Device SoftAP HTTP API in Configuration Mode | [iconnect/README.md](iconnect/README.md) |

## How the parts connect

```
Android phone ── Wi-Fi SoftAP, HTTP ──┐
(iconnect)      (Configuration Mode)  │
                                      ▼
sensor cluster ── RS-485 ──▶ ESP32-C6 ── UART ──▶ ESP32-S3
(sensor)       Modbus RTU    controller           (ai)
                                 │
                                 └── MQTT ──▶ broker, trains the model offline
```

* The C6 is the master of both wired links: it polls the sensor cluster and streams data to the S3.
* The S3 is off on battery. The sensor cluster is powered from the main box 5 V pair.
* The phone only talks to the C6, and only while it is in Configuration Mode.

## Related files outside this directory

| Path | Content |
|---|---|
| `../hardware/sensor-cluster/` | KiCad project and production files for the sensor cluster board |
| `../hardware/mainboard/` | Main board, including the RS-485 master end |
| `../components/hmodbus/` | C6 Modbus RTU master, the counterpart of the sensor cluster firmware |
| `../components/hwifi/` | C6 SoftAP and HTTP server that `iconnect` talks to |
| `../doc/stractures.md` | Wire protocol layout |
| `../README.md` | Full design: power, placement, pin tables |

## Conventions

* One README per project holds its design notes and open items. Keep wiring facts in the hardware
  README or the schematic, and link to them instead of copying.
* Build output (`build/`, `.gradle/`, `local.properties`) stays out of git.
