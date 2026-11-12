# IrrigateConnect
* Code build by ClodeCode

Android companion app for the Smart Irrigate controller. When the device is in
Configuration Mode it starts a SoftAP + local HTTP API; IrrigateConnect connects to that AP and
talks to the API to read and push Wi-Fi/MQTT setup values, without needing the device on your
normal network yet.

## What it does

- On launch, silently probes the device (`GET /api/status`) and, if reachable, loads its current
  setup key/value pairs as editable fields (text, number, or boolean depending on the JSON value
  type).
- Lets you edit those fields and push them back with `POST /api/set` (as an INI-style body,
  `[setup]\nkey=value\n...`), then re-fetches and verifies the device actually accepted each value.
- Optionally restarts the device (`POST /api/restart`) after a successful apply.
- Also fetches static device details (`GET /api`, e.g. firmware/esp-idf version) on launch and shows
  them read-only in an **About** dialog, reachable from the top bar's overflow (⋮) menu — which is
  also where a **Settings** entry will live once there's device-wide configuration to expose.

## Talking to the device

`DeviceApi` (`app/src/main/java/org/magicat/irrigate/connect/network/DeviceApi.kt`) is the only
place that makes HTTP calls, against a base URL built from `BuildConfig.DEVICE_HOST` /
`DEVICE_PORT`:

| Call | Method | Endpoint | Purpose |
|---|---|---|---|
| `fetchInfo` | GET | `/api` | Static device details (read-only) |
| `fetchStatus` | GET | `/api/status` | Current setup key/value pairs |
| `pushStatus` | POST | `/api/set` | Apply new setup values (INI body) |
| `restartDevice` | POST | `/api/restart` | Reboot the device |

The device's JSON responses use `key=value` pairs inside `{}` rather than standard `key:value`;
`org.json.JSONObject`'s tokenizer accepts `=` as a colon substitute, so this parses fine as-is —
don't "fix" it to `:` without also checking the firmware side.

Default target is the device's SoftAP gateway (`192.168.3.1:80`, see `DEVICE_HOST`/`DEVICE_PORT` in
`app/build.gradle.kts`) — connect your phone to the device's AP before using the app, or change
those build config fields to point at a device already on your LAN.

## Build

Standard Gradle/AGP project, no ESP-IDF toolchain needed:

```bash
./gradlew assembleDebug
# or, connected device/emulator:
./gradlew installDebug
```

Requires `compileSdk`/`targetSdk` 37, `minSdk` 30 (see `app/build.gradle.kts`).

## Project layout

```
app/src/main/java/org/magicat/irrigate/connect/
├── MainActivity.kt        # Compose UI: connect/refresh, settings list, About dialog
├── model/ConfigValue.kt   # JSON <-> ConfigField mapping, INI serialization
├── network/DeviceApi.kt   # HTTP client for the device's setup API
└── ui/theme/              # Compose theme
```
