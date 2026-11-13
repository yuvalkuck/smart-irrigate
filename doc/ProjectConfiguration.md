# Project Configuration

[← Back to the main README](../README.md)

## Hardware Pin Configurations
The physical hardware mapping on the ESP32-C6 micro-controller uses compile-time variables defined via Kconfig:
* **System Boot Switch:** **GPIO 23**. Configured as a digital input relying on internal pull-up resistors. A logical `LOW` reading recorded during the power-on sequence intercepts standard execution loops to force execution into Configuration Mode.
* **Shared I2C Bus:** **GPIO 19 (SDA)** and **GPIO 20 (SCL)**. This digital serial bus multiplexes data extraction lines from the local environment sensors.
  * *SHT41 Sensor:* Provides precision ambient temperature and relative humidity parameters via fixed I2C signaling.
  * *BMP581 Sensor:* Transmits targeted barometric pressure metrics and ambient temperature data.
  * *ADS1115 ADC:* Reads line pressure, wind speed and board temperature (see "Analog Interface Architecture"). Address 0x48 (ADDR pin on GND).
  * *INA226 monitor:* Reads the battery bus voltage and the battery current through a 10 mohm shunt. Address 0x40 (A0 and A1 on GND).
  * *TCA9555 I/O expander:* Drives the status LEDs (P00 to P06), reads the SHOW LEDs button (P10) and drives the 24 VAC converter enable (P12). Address 0x20 (A0, A1 and A2 on GND).
  * *Note:* All the devices function on the exact same physical GPIO 19 and 20 paths, utilizing unique hardware address layers to prevent data line collisions.
* **RS-485 Link to the Sensor Cluster:** **GPIO 9 (TX)**, **GPIO 8 (RX)** and **GPIO 15 (DE and /RE)**, through a MAX3485 to terminal J5 (1 = A, 2 = B, 3 = +5 V, 4 = GND).
* **1-Wire Serial Interface:** **GPIO 4**. Configured as an open-drain bidirectional digital line with a dedicated external pull-up resistor.
  * *DS18B20 Sensor:* Provides high-accuracy underground soil thermal parameters using the precision timing 1-Wire protocol.
  * *Board temperature (U7):* An MCP9700A analog sensor on ADS1115 AIN2, not on the 1-Wire bus. Output is 500 mV at 0 degC plus 10 mV per degC. **TBD:** the firmware does not read it yet.
* **Analog Interface Architecture:** All analog inputs are read by one **ADS1115** 16-bit ADC on the shared I2C bus (GPIO 19 and 20), not by the ESP32-C6 internal ADC, which is noisy and non-linear. No C6 GPIO is used for analog signals. Channel order (as in the schematic): AIN0 XDB401 pressure, AIN1 wind speed, AIN2 board temperature (MCP9700A), AIN3 unused. Battery voltage is read by the INA226, not the ADS1115.
  * *Battery voltage and current:* Read by the INA226 over I2C (bus voltage from the 24 V bus, current from the 10 mohm shunt R31 in the battery negative lead), so no divider into the ADS1115 is needed.
  * *XDB401 Pressure Transmitter:* Records real-time water line pressure, with hardware conditioning to scale the output signal to fit within the ADS1115 input limits.
  * *Analog Wind Speed Sensor:* Because the sensor outputs a 0–5V range, an external voltage divider circuit steps the incoming voltage down to a safe, readable level. The software maps this reading back to the true 0.0–30.0 meters per second wind speed curve.
  * *Input protection (each channel):* A divider so a pin never exceeds 3.3 V, a 1 to 10 kohm series resistor, a clamp to 3.3 V and GND at the pin, and a 10 to 100 nF capacitor to GND. The ADS1115 is powered from the 3.3 V rail only, with 100 nF at VDD.
  * **TBD:** valve fail-safe when the ADS1115 stops answering, for example the "No Water Pressure" check cannot run. Either close the valve, or run it on a timer only.
* **Valve Relays:** **GPIO 1, 14, 18, 21, 22, 16, 17, 5** for valves 1-8, declared as `gpio_valve_0` to `gpio_valve_7` in `components/board_manager/board_peripherals.yaml`, with `CONFIG_DEVICE_MAX_VALVES` = 8. The FireBeetle 2 ESP32-C6 header has no GPIO 10 or 11. GPIO 16/17 are UART0, so the console must run on USB Serial/JTAG. GPIO 8/9 (boot strapping) and 15 (on-board LED) are not used for valves. The 8 valve GPIOs reach the relay module through two SN74AHCT125N buffers (U10 for valves 1-4, U11 for valves 5-8), which raise the 3.3 V GPIO level to 5 V. More than 8 valves needs an I2C GPIO expander.
* **Inter-Chip Link (ESP32-S3 Connection):** **GPIO 6 (TX)** and **GPIO 7 (RX)**. A UART link connecting the FireBeetle 2 to a companion ESP32-S3 module, with GPIO 6 wired to the ESP32-S3's RX pin and GPIO 7 to its TX pin.
* **LCD2004 (on the ESP32-S3):** the display and its wake button are wired to the ESP32-S3, not the C6. The LCD is a 5 V module with an I2C backpack (PCF8574T) whose pull-ups are tied to 5 V, so a BSS138 level shifter (U9) sits between the 3.3 V S3 I2C lines and the backpack. It is powered from the 5 V net, the same net as the S3 5 V input, so it follows the S3 supply and needs no power switch of its own. It is a mains-only feature (the S3 is off on battery). Proposed S3 pins: I2C SDA GPIO 8, SCL GPIO 9, wake button GPIO 4 (avoiding the S3's strapping pins 0/3/45/46, USB pins 19/20 and the octal-PSRAM pins 35-37 on N16R8 boards). **TBD:** confirm when the S3 is added.

## Network & Protocol Configurations
The network architecture is configured natively under the revised ESP-IDF 6.1 components using the following specifications:
* **Wi-Fi Subsystem:** Tailored to exploit the ESP32-C6 radio. It hooks into the global `esp_event` loop framework to transition automatically between the SoftAP + HTTP server configuration topology and the automated Station network connector profile.
* **Internet Time Synchronization (SNTP):** Utilizing the native **`esp_netif_sntp` framework** optimized in ESP-IDF 6.1. Upon establishing an active station connection in Operational Mode, the system queries public Network Time Protocol (NTP) pools via network sockets to configure and adjust the internal hardware Real-Time Clock (RTC). This guarantees millisecond-accurate scheduling logs and execution timestamps for all 48 optional irrigation events without needing a local hardware RTC battery module.
* **MQTT Client Configuration:** Powered by the core communication component, it parses data over two primary pipelines:
    * *Telemetry & Event Topic (Outbound):* A target path used to broadcast serialized data detailing live water pressure, wind speed in meters per second, ambient temperature, humidity, and immediate valve state event logs.
    * *Command/Configuration Topic (Inbound):* A real-time subscription pathway that intercepts remote instructions, environmental baseline thresholds, historical seasonal data packets, and the 6-event scheduler layouts for each of the 8 compiled valves.

## Storage & Partition Layout Architecture
To optimize access speed, reduce wear overhead, and safely isolate temporary networking properties from large irrigation parameters, flash memory storage is separated into distinct custom partitions within the `partitions.csv` topology:
* **Communications NVS Partition (`setup`):** A dedicated, standard key-value NVS flash space strictly reserved for storing Wi-Fi credentials (SSID, Password), security flags, SNTP server pool parameters, and primary MQTT broker socket configuration strings. This partition is exclusively rewritten during Configuration Mode.
* **Irrigation Storage Data Partition (`config`):** A separate, dedicated data flash partition optimized to hold complex valve schedule configurations, the 48 optional irrigation events, dynamic operational parameters, and historical wind and environmental baseline metrics.
