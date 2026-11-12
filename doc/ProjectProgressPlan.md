# Project Progress Plan

[← Back to the main README](../README.md)

**Current status:** a half-working prototype on a breadboard, built from off-the-shelf modules.

**Firmware (ESP32-C6) progress:**

* Done, builds, not yet run on a device:
  * Sensor platform source over Modbus (`SensorPlatform`). It is switched by the `PlatformSensor` bit of the hardware enable mask, read once at boot. The register map is `components/protocol/include/platform_modbus.h`.
  * Board temperature: MCP9700A on ADS1115 AIN2, through the new `ads1115` component, and sent as the last field of the MQTT telemetry string.
* Not started: valve engine and scheduler, configuration parser and command dispatch, "No Water Pressure" handling, valve and fault events, STM32 sensor platform firmware, power modes, ESP32-S3 link.

| Stage | Goal | Tasks |
|---|---|---|
| **1. Breadboard** (now) | Prove the sensors and the valve control | Finish the breadboard prototype. Pick and test the 24 VAC converter with a real valve (coil heat, buzz). |
| **2. Firmware for the power design** | Implement what the Power Architecture describes | Mains-detect and battery mode, the mains valve limit, the battery round robin, Wi-Fi and S3 off on battery, a maximum open time per valve. |
| **3. Boxes** | Move into the real enclosures | Main box, battery box, sensor mast, roof mast, soil probe and water-line fitting (see "Physical Placement"). Fuses, an RCD on the mains side, correctly rated enclosures, a manual shutoff valve on the water line. |
| **4. Live test run** | Run the system for real, even without the large battery | Failure tests: power cut, Wi-Fi loss, no water pressure, a dead sensor, a reboot mid-watering (the valves must close in every case). Measure the real power draw to check the estimates, then size the battery. Confirm OTA updates or serial access with the boxes closed. |
| **5. Outdoor soak test** | Find weather and drift problems | A few weeks outdoors: condensation, heat in the main box, UV on the cables. Calibrate the wind and pressure sensors. |
| **6. PCB** | Replace the modules with a custom board | Freeze the design first. Version 1: a main board with ESP32 WROOM modules, relays, power conversion and connectors for the remote sensors, plus a small satellite board for the SHT41 on the mast. Keep the 230 V PSU a separate module. |
