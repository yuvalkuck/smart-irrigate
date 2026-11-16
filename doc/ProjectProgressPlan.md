# Project Progress Plan

[← Back to the main README](../README.md)

**Current status:** the breadboard stage is finished. The work is now on the PCB design.

## How to read and update this list

All tasks are in one table, in the order of their progress. Read it top to bottom: the past is at the top, the future is at the bottom.

**Statuses**

| Status | Meaning |
|---|---|
| Done | Finished. Past. |
| Progress | Being worked on now. Present. |
| Plan | Not started. Future. |
| Cancelled | Dropped. Past. |

**Order rules**

1. The top of the table is the closed block: every Done and Cancelled row, in the order they were closed, oldest first.
2. A closed row is fixed. It keeps its place and never moves, so the history stays readable.
3. When a task becomes Done, it goes to the end of the closed block, right under the last Done or Cancelled row.
4. When a Progress or Plan task is cancelled, it also goes right under the last Done or Cancelled row.
5. Under the closed block come the Progress rows, then the Plan rows. These rows can be reordered or moved up and down as the work changes.
6. A new task is added as Plan. When work on it starts, change its status to Progress.

**Columns**

* **Status:** one of the four statuses above.
* **Task:** a short name.
* **Goal:** what the task includes, in more detail. It may be empty.

## Tasks

| Status | Task | Goal |
|---|---|---|
| Done | Split `platform/` | `iconnect`, `sensor` and `ai` under `platform/`. |
| Done | Unit tests moved to `utest/` | GoogleTest smoke test, built on the host. |
| Done | ESP Board Manager | Pins in `components/board_manager/board_peripherals.yaml`. It replaces the hand-written GPIO setup and the `CONFIG_DYNAMIC_VALVE_GPIO_LIST` option. |
| Done | Mainboard schematic | ADS1115, MCP9700A, INA226, TCA9555 I/O expander, level shifter, status LEDs and a button. |
| Done | Platform Sensors board | STM32C011, MAX3485, solar input, protection, LEDs and test points. SHT41 and BMP581 are external. |
| Done | NVS rework | Simpler keys. The sensor enable masks are stored as a blob. |
| Done | Modbus link to the Platform Sensors | `hmodbus` RTU master and `SensorPlatform`. It is switched by the `PlatformSensor` bit of the hardware enable mask, read once at boot. The register map is `components/protocol/include/platform_modbus.h`. Builds, not yet run on a device. |
| Done | Board temperature | MCP9700A on ADS1115 AIN2, through the `ads1115` component. Sent as the last field of the MQTT telemetry string. Builds, not yet run on a device. |
| Done | More details on the GitHub project | |
| Done | Breadboard | Only test how it works, so the PCB can be changed if needed. Prove the sensors and the valve control. Pick and test the 24 VAC converter with a real valve (coil heat, buzz). |
| Progress | PCB | Close the open HW issues, then order a PCB to work with. That is why more and more parts are added to the board. The steps: layout, router, send it to print, then soldering. Version 1 of both boards: the mainboard carries the FireBeetle 2 ESP32-C6 and the ESP32-S3 DevKitC-1, the power conversion, the header for the off-board relay module (J15) and the RS-485 connector for the remote sensors; the Platform Sensors board (STM32C011) is a satellite board on the sensor mast and carries an SHT41 module. Keep the 230 V PSU a separate module. |
| Progress | System integration | Make the parts work together as one system. The board needs the power, the relays and the sensor connections, so all the PCB issues are part of it. It includes the partitioning: the split into the platform (Platform Sensors) and the board (mainboard). Last step: move into the real enclosures (main box, battery box, sensor mast, roof mast, soil probe and water-line fitting, see "Physical Placement") with fuses, an RCD on the mains side, correctly rated enclosures and a manual shutoff valve on the water line. |
| Plan | Wind sensor: select where it connects | HW task. The wind sensor stays on the mainboard (ADS1115 AIN1). Add the option to connect it through the Platform Sensors instead, and to select which one is used. The selection is in the HW enable mask (NVS), with two options: `SensorWindBoard` (0x100) and `SensorWindPlatform` (0x200). Select none or one of them. |
| Plan | iconnect | Android app in `platform/iconnect`, works over Wi-Fi. It can change all NVS parameters, including the HW and SW enable masks. The exception: with AI disabled in menuconfig (`DEVICE_PLATFORM_AI_ENABLE`), it cannot change it. Not yet confirmed in code. Own session. |
| Plan | INA226 firmware | Write the I2C driver for the INA226 at 0x40 (the part is on the mainboard, there is no driver yet). Read the battery bus voltage and the battery current through the 10 mohm shunt, for coulomb counting and a low-battery warning. |
| Plan | TCA9555 driver: LEDs, button, 24 VAC enable | Write the I2C driver for the expander at 0x20 (the part is on the mainboard, there is no driver yet). Drive the 7 status LEDs (P00 to P06), read the SHOW LEDs button (P10), switch the 24 VAC converter enable (P12) only while a valve is open. What each LED shows: not defined yet. |
| Plan | Valve engine and scheduler | Run the 8 valves, each with its own logic and up to 6 water events (48 blocks in total). Each event is evaluated against the MQTT operational profile and adjusted by the environmental compensation (temperature, humidity, solar load, wind, pressure trend). |
| Plan | Configuration parser and command dispatch | Parse the binary `Command` and `Configuration` payloads received over MQTT (`doc/stractures.md`) and apply them: the profiles in the `config` partition, and the restart command. |
| Plan | "No Water Pressure" handling | When a valve is open and the XDB401 reports no water pressure, close the valve, log a fault event to MQTT, defer the irrigation cycle and poll the line until pressure returns, then resume. Open question: the valve fail-safe when the ADS1115 stops answering (close the valve, or run on a timer only). |
| Plan | Valve and fault events | Valve and fault events published to the MQTT tracking server, for example the no-pressure fault. The list of events is not defined yet. |
| Plan | STM32 Platform Sensors firmware | Firmware for the STM32C011 (STM32CubeMX project `platform/sensor/sensor_platform.ioc`). Modbus RTU slave at address 1, 9600 baud 8N1, on the RS-485 link. It answers function 0x04 and 0x03 with the SHT41, BMP581 and solar (PA0) readings, and answers a bad request with a Modbus exception (`platform/sensor/README.md`). |
| Plan | ESP32-S3 link | UART link between the C6 (GPIO 6 TX, GPIO 7 RX) and the companion ESP32-S3 (AI). The C6 streams the microclimate and water-supply data, the S3 runs the inference and adapts the irrigation configuration. The S3 is off on battery. |
| Plan | Firmware for the power design | Implement what the Power Architecture describes. Mains-detect and battery mode, the mains valve limit, the battery round robin, Wi-Fi and S3 off on battery, a maximum open time per valve. |
| Plan | Live test run | Run the system for real, even without the large battery. Failure tests: power cut, Wi-Fi loss, no water pressure, a dead sensor, a reboot mid-watering (the valves must close in every case). Measure the real power draw to check the estimates, then size the battery. Confirm OTA updates or serial access with the boxes closed. |
| Plan | Outdoor soak test | Find weather and drift problems. A few weeks outdoors: condensation, heat in the main box, UV on the cables. Calibrate the wind and pressure sensors. |
