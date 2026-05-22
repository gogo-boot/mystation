# GitHub Copilot Instructions — MyStation

## Project Context
ESP32 firmware (Arduino/PlatformIO) for an e-paper public transport departure board.
Displays RMV (German transit) departures and DWD weather data on a 800×480 GDEY075T7 e-paper display.
Targets ESP32-C3 and ESP32-S3 hardware.

## Build System
- **PlatformIO** — use `pio run`, `pio test`, `pio run --target upload/uploadfs`
- Default env: `esp32-s3-e1001-production` (see `platformio.ini`)
- Board variants: `esp32-c3-{debug,production}`, `esp32-s3-e1001-{debug,production}`, `esp32-s3-ee04-{debug,production}`
- Feature flags detected in `include/build_config.h`: `BOARD_ESP32_C3`, `PCB_E1001`, `PCB_EE04` → drive `SHOW_BATTERY_STATUS`, `HAS_BUTTON`
- `PRODUCTION=0|1` → controls `CORE_DEBUG_LEVEL` and debug display overlays

## Boot Architecture
- **All logic runs inside `setup()`** via `ActivityManager` — do NOT put logic in `loop()`
- Lifecycle sequence: `ON_INIT → ON_START → ON_RUNNING → ON_STOP → ON_SHUTDOWN` → deep sleep
- `ON_LOOP` is only used for WiFi config mode (web server runs inline in `setup()`)
- Device deep sleeps after every normal cycle; `wakeupCount` is `RTC_DATA_ATTR` and persists

## Configuration Phases (`include/config/config_struct.h`)
- `PHASE_WIFI_SETUP` → `PHASE_APP_SETUP` → `PHASE_COMPLETE`
- NVS (`Preferences`) persists settings across deep sleep; `ConfigOption` struct is RAM-only (rebuilt each wakeup)

## Global Instances Pattern
Defined **once** in `main.cpp`, declared `extern` in `include/global_instances.h`:
- `display` — GxEPD2 e-paper driver
- `u8g2` — UTF-8 font renderer (required for German umlauts ä/ö/ü/ß)
- `server` — WebServer for config mode
- `wakeupCount` — RTC-persistent wakeup counter

## Display Rendering Rules
- GxEPD2 uses **paged drawing** — always wrap render calls:
  ```cpp
  display.setFullWindow();
  display.firstPage();
  do { /* draw here */ } while (display.nextPage());
  ```
- Three display modes: half&half (weather + departures), weather-only, departures-only
- Display code lives in `src/display/`

## Testing
```bash
pio test -e native -v     # Run native tests (no hardware needed)
./run_test.sh             # Same with summary output
```
- `test/mocks/` provides Arduino/ESP32 stubs (`Arduino.h`, `Preferences.h`, `esp_log.h`, etc.)
- Only `test_timing_manager` compiles natively; API tests parse JSON from `test/{api}/` fixture files
- Add `-DNATIVE_TEST` guard around hardware-dependent code in new files

## Project Conventions
- **Size matters**: use `-Os`, avoid heap allocations, prefer stack-local `ArduinoJson` documents
- **German locale**: UI strings may be German; WiFiManager strings overridden via `i18n/wm_strings_de.h`
- **FIRMWARE_VERSION**: from `git describe --tags` in production; static string in debug builds
- **API keys**: live in `include/secrets/` (not committed) — never hardcode keys elsewhere
- **TLS cert** for OTA: `cert/github_bundle.pem` embedded at build time via `board_build.embed_txtfiles`
- Use `DEBUG_ONLY(x)` / `PRODUCTION_ONLY(x)` macros from `include/build_config.h` for conditional code

## Key Files
| File | Role |
|------|------|
| `src/main.cpp` | Entry point, global instance definitions |
| `include/global_instances.h` | Extern declarations for shared globals |
| `include/build_config.h` | Feature flags, board detection, debug macros |
| `include/config/config_struct.h` | `ConfigOption` struct, `ConfigPhase` enum |
| `src/activity/` | `ActivityManager` lifecycle implementation |
| `src/display/` | E-paper rendering (mode-specific files) |
| `src/api/` | RMV, DWD, Google API clients |
| `platformio.ini` | All build environments and flags |

