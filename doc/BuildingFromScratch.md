# Building from Scratch

[← Back to the main README](../README.md)

These steps take a clean machine and a fresh `git clone` to a flashed board. They were verified on Debian 13 with
ESP-IDF v6.1.0 by building a copy of the repository that had no `build/`, `managed_components/`, `components/gen_bmgr_codes/`
or `certs` content.

## Build dependencies

**Tools (host machine)**

| Tool | Why |
|---|---|
| `git`, `wget` | clone the project and ESP-IDF |
| Python 3 (with `pip` and `venv`) | ESP-IDF tooling; `install.sh` creates its own virtual environment |
| `cmake`, `ninja-build` | the ESP-IDF build system |
| `openssl` | generating the MQTT TLS certificates (step 4) |
| `flex`, `bison`, `gperf`, `libffi-dev`, `libssl-dev`, `dfu-util`, `libusb-1.0-0` | ESP-IDF host prerequisites |
| `ccache` (optional) | faster rebuilds |

On Debian or Ubuntu:

```bash
sudo apt install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache \
    libffi-dev libssl-dev dfu-util libusb-1.0-0 openssl
```

**ESP-IDF**: v6.1. The project was last built with v6.1.0. The ESP32-C6 toolchain
(`riscv32-esp-elf`) is installed by ESP-IDF's `install.sh`.

**ESP-IDF components**: downloaded automatically by the IDF Component Manager during `idf.py reconfigure`, so the first
configure needs network access. They are pinned in `dependencies.lock`:

| Component | Used for |
|---|---|
| `espressif/mqtt` | MQTT client |
| `espressif/esp-modbus` | Modbus RTU master (RS-485 link to the sensor cluster) |
| `espressif/esp_board_manager` | pin and peripheral declarations (`components/board_manager`) |
| `espressif/ds18b20`, `espressif/onewire_bus` | soil temperature probe |
| `k0i05/esp_sht4x`, `esp-idf-lib/i2cdev` | SHT4x I2C sensor |
| `espressif/fmt` | formatting |

The Bosch BMP5xx driver is vendored in `components/bmp5api`; nothing to install for it. The `esp-bmgr-assist` pip package is
not needed.

**Hardware**: DFRobot FireBeetle 2 ESP32-C6 and a USB cable. On Linux your user must be allowed to open the serial port
(`sudo usermod -aG dialout $USER`, then log in again).

**Optional**: Docker (the local Mosquitto brokers under `test/mosquitto/`), and a host compiler with CMake for
the host-side tools in `test/genpaylaod/` and `utest/`. `utest/` downloads GoogleTest at configure time.

## Steps after a fresh clone

1. **Install ESP-IDF** (once per machine):

   ```bash
   mkdir -p ~/esp && cd ~/esp
   git clone -b v6.1 --recursive https://github.com/espressif/esp-idf.git
   cd esp-idf && ./install.sh esp32c6
   ```

2. **Load the ESP-IDF environment** in every new shell:

   ```bash
   . ~/esp/esp-idf/export.sh
   ```

3. **Clone the project:**

   ```bash
   git clone <repository-url> smart-irrigate
   cd smart-irrigate
   ```

4. **Generate the MQTT TLS certificates.** The firmware embeds `ca.crt`, `client.crt` and `client.key` from `certs/`
   (a symlink to `test/mosquitto/config/certs`, which is git-ignored), so they must exist before CMake runs.
   Open `test/mosquitto/config/generate_certs.sh`, set `IP_OR_HOSTNAME` to your broker's address, then run it from its own
   directory:

   ```bash
   cd test/mosquitto/config && ./generate_certs.sh && cd ../../..
   ```

   The broker must be configured with the matching `server.crt`/`server.key` and CA. To use certificates from your own CA,
   place `ca.crt`, `client.crt` and `client.key` in `test/mosquitto/config/certs/` instead.

5. **(Optional) Bake in first-boot defaults.** `components/common/gen/default_initiate.in.h` is filled from environment
   variables at configure time: `INITIATE_WIFI_SSID`, `INITIATE_WIFI_PASSWORD`, `INITIATE_DEVICE_UNIQUE`,
   `INITIATE_BROKER_URL`, `INITIATE_MQTT_CLIENT_USERNAME`, `INITIATE_MQTT_CLIENT_PASSWORD`, `INITIATE_NTP_SERVER`,
   `INITIATE_DEFAULT_LOCALE_TIME_ZONE`. Export them before step 6. Unset variables become empty strings and the device is
   provisioned over the SoftAP instead (see [Setup & Operational Mode](../README.md#setup--operational-mode)).

6. **Fetch the managed components and configure:**

   ```bash
   idf.py reconfigure
   ```

7. **Generate the board code** from `components/board_manager/*.yaml`. `components/gen_bmgr_codes/` is generated and
   git-ignored, so this step is required after every clone and after every change to the YAML files:

   ```bash
   idf.py bmgr -b board_manager
   ```

8. **Build:**

   ```bash
   idf.py build
   ```

9. **Flash and monitor** (replace the port):

   ```bash
   idf.py -p /dev/ttyACM0 flash monitor
   ```

   Exit the monitor with `Ctrl+]`.

Notes:

* `sdkconfig` is committed and holds the project's settings (custom partition table, 4 MB flash, `-Werror` relaxed).
  Do not delete it. Change settings with `idf.py menuconfig`.
* The target is fixed to `esp32c6` in the root `CMakeLists.txt`; `idf.py set-target` is not needed.
* Pins and peripherals are declared in `components/board_manager/board_peripherals.yaml`. Board constants that are not pins
  (RS-485 UART port, wind ADC channel) are in `hardware/boards/<BOARD>.cmake`; select another file with
  `idf.py -DBOARD=<name> reconfigure`.

## Troubleshooting a fresh build

| Symptom | Cause and fix |
|---|---|
| `CMake Error in components/hmqtt/CMakeLists.txt: Cannot find source file: .../certs/ca.crt` | the certificates do not exist yet; do step 4 |
| `[ESP_BMGR_ASSIST] Error: missing generated board artifacts in components/gen_bmgr_codes` | the board code was not generated; do step 7 |
| `-Werror=missing-field-initializers` errors in `hwifi/soft_ap.cpp` | `sdkconfig` is missing or was regenerated from defaults; restore it with `git checkout -- sdkconfig` |
