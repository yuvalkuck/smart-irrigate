# components/

Gotchas per component. Layout and dependencies are in each component's `CMakeLists.txt`.

- **`flash`** — the `ENABLE_HW`/`ENABLE_SW` bit masks (`HardwareEnable`/`SoftwareEnable`) that switch
  sensors and features are read once at boot, so a change needs a restart.
- **`hmqtt`** — certs are embedded from the repo-root `certs/` symlink; see the root `CLAUDE.md` for
  the files that must exist before building.
- **`telemetry`** — sensor drivers implement one of the `SensorConcept`/`SensorI2C`/`SensorADC`/
  `SensorOneWire` C++20 concepts in `telemetry/inc/sensor_concept.h` (compile-time-enforced via
  `AbstractSensorI2C<Derived>` etc. — every sensor needs `init(bus)`, `online(bus)`, and
  `read(TelemetryData&)`). New sensors follow this pattern rather than being wired ad hoc into
  `telemetry.cpp`. The remote sensor platform (SHT41, BMP581, solar) is read by `SensorPlatform` over
  Modbus when the `PlatformSensor` mask bit is set. The board temperature (MCP9700A on ADS1115 AIN2)
  is `SensorBoardTemp`.
- **`protocol`** — structs are `#pragma pack`ed and guarded with `#if !defined(TESTER)` so the same
  headers compile both on-device and in the host `test/genpaylaod` tool; keep new protocol code
  TESTER-safe. **The wire format is actively evolving** — `doc/stractures.md` documents the
  currently-implemented `Command`/`Configuration` layout; treat anything not reflected there as not
  yet implemented. `platform_modbus.h` holds the Modbus register map shared with the sensor platform
  (`platform/sensor`).
- **`bmp5api`** — vendored Bosch BMP5xx sensor API (upstream driver, avoid modifying — see its own
  `README.md`/`LICENSE` under `components/bmp5api/1.5.0/`).
