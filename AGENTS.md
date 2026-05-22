# AGENTS.md — MyStation Codebase Guide

## Project Overview
ESP32 firmware (Arduino/PlatformIO) for an e-paper public transport departure board. Displays RMV (German transit) departures and DWD weather data. Targets ESP32-C3 and ESP32-S3 hardware.

## Build & Flash
```bash
# Default env: esp32-s3-e1001-production
pio run --target upload       # Flash firmware
pio run --target uploadfs     # Flash LittleFS (data/ directory → config HTML page)

# Target a specific env
pio run -e esp32-c3-debug --target upload
```

**Environments** (`platformio.ini`):
- `esp32-c3-{debug,production}` — C3 SuperMini, no battery monitor, no buttons
- `esp32-s3-e1001-{debug,production}` — S3 PCB E1001, battery + buttons
- `esp32-s3-ee04-{debug,production}` — S3 PCB EE04 variant

**Board feature flags** set via build_flags, detected in `include/build_config.h`:
- `BOARD_ESP32_C3`, `PCB_E1001`, `PCB_EE04` → controls `SHOW_BATTERY_STATUS`, `HAS_BUTTON`
- `PRODUCTION=0|1` → controls `CORE_DEBUG_LEVEL`, debug display overlays

## Running Tests
```bash
pio test -e native -v          # Run all native (desktop) tests
./run_test.sh                  # Same but with summary output
```
Tests in `test/` use mocks in `test/mocks/` (e.g., `Arduino.h`, `Preferences.h`, `esp_log.h`) to compile without hardware. Only `test_timing_manager` runs on native; other tests (`rmv/`, `dwd_weather/`, etc.) exercise JSON parsing using test data in `test/{api}/`.

## Architecture

### Boot Lifecycle (key pattern)
`main.cpp` runs everything inside `setup()` via `ActivityManager`. The `loop()` is only a fallback. Lifecycle states: `ON_INIT → ON_START → ON_RUNNING → ON_STOP → ON_SHUTDOWN` (then deep sleep), or `ON_LOOP` for WiFi config mode (web server runs inline).

### Configuration Phases (`include/config/config_struct.h`)
1. `PHASE_WIFI_SETUP` — no WiFi credentials yet
2. `PHASE_APP_SETUP` — WiFi OK, app settings (stop, schedule) needed
3. `PHASE_COMPLETE` — operational; fetches data, renders display, sleeps

Config persists in **NVS** (Non-Volatile Storage) via `Preferences`. `ConfigOption` struct lives in RAM only; rebuilt after each wakeup.

### Global Instances (`include/global_instances.h`)
Defined once in `main.cpp`, declared `extern` everywhere else:
- `display` — GxEPD2 800×480 e-paper (GDEY075T7)
- `u8g2` — UTF-8 font renderer (needed for German umlauts)
- `server` — WebServer for config mode
- `wakeupCount` — `RTC_DATA_ATTR`, persists across deep sleep

### Key Directories
| Path | Purpose |
|------|---------|
| `src/activity/` | Lifecycle orchestration |
| `src/api/` | RMV, DWD, Google API clients |
| `src/config/` | NVS config load/save, web config page |
| `src/display/` | E-paper rendering per mode (half&half, weather-only, transport-only) |
| `src/ota/` | OTA update scheduling |
| `include/secrets/` | API keys (not committed; must be created locally) |
| `data/` | LittleFS filesystem — config HTML page uploaded via `uploadfs` |
| `test/mocks/` | Arduino/ESP32 stubs for native test builds |

## Project-Specific Conventions
- **German locale**: WiFiManager strings overridden via `-DWM_STRINGS_FILE` → `i18n/wm_strings_de.h`; display text may be German.
- **FIRMWARE_VERSION**: injected via `git describe --tags` in production builds; use static fallback in debug (`version_flag_static`).
- **Size optimization**: All device builds use `-Os -ffunction-sections -fdata-sections -Wl,--gc-sections`; avoid large dynamic allocations.
- **Deep sleep is the norm**: code must complete within `setup()`; do not rely on `loop()` except for web server config mode.
- **Display page rendering**: GxEPD2 uses paged drawing. Always wrap render calls in `display.setFullWindow()` / `display.firstPage()` … `display.nextPage()` loops.

## External API Dependencies
- **Google Geolocation API** — location from WiFi scan (key in `include/secrets/`)
- **RMV API** — departure board for Hesse, Germany (key in `include/secrets/`)
- **DWD (Deutscher Wetterdienst)** — weather, no key required (open API)
- GitHub releases endpoint — OTA firmware check (`cert/github_bundle.pem` embedded for TLS)

