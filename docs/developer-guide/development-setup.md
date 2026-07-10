# Project Set-Up Guide

### Prerequisites

- Software Requirements:
    - **PlatformIO IDE** (VS Code extension) or CLion with PlatformIO plugin
    - **Git** for version control
- Hardware Requirements:
    - **ESP32** Development Board
        - It is tested with ESP32-C3 and ESP32-S3 series
    - E-Paper Display : those are exact same. I guess, Seeed resell the Good Display one.
        - GDEY075T7 : Good display 7.5" e-Paper Display (800x480)
        - or Seeed Studio 7.5" Monochrome eInk (SKU 105990172)
    - Display Hat / Driver Board
        - Good Display DESPI-C02, 24 PIN 0,5 mm, 3.3V
        - or Seeed Studio ePaper Breakout Board (SKU 105990172)
    - Rechargeable Battery (Optional)
        - For portable use
        - LiPo or Li-Ion battery with appropriate voltage and capacity
        - Strongly recommended to get it with BMS (Battery Management System) for safety
    - Battery Charging Circuit (Optional)

### Recommended Hardware

I recommend using the `TRMNL 7.5" (OG) DIY Kit (SKU 104991005)` from Seeed Studio

- Includes the e-Paper display,
- Includes tESP32-S3 Plus
- Includes tE-Paper Display
- Includes tDisplay Hat
- Includes rechargeable battery with BMS
- It supports also charging Battery via USB-C. No need to buy charging circuit separately.
- You can also measure the voltage of the battery and get current battery level.
- You don't need to solder anything, just assemble the parts.

## Software Setup

- Install Platform IO
- Git Clone this repository
- Get and Set API Keys (see below)

## Getting API Keys

- Get API keys for:
    - Google Geolocation API : This API is used to get current location from Wifi access points.
        - Get API key from https://console.cloud.google.com/apis/credentials
    - RMV Public Transport API : This API is used for getting departure/ arrival times for stations in the Rhein-Main
      area.
        - Get API key from https://opendata.rmv.de/site/start.html
    - Open Meteo Weather API : This API is used for getting weather data
        - No API key required for non-commercial use
    - OpenStreetMap Nominatim : This API is used for getting location names from coordinates.
        - Location names (free, no key)

for detail, see [API Integration Guide](./api-integration.md)

### Setup API Keys

`include/secrets/general_secrets.h` file is excluded from git for security reasons.
you need to create these files manually based on the provided examples `include/secrets/general_secrets.h.example`.

```bash
cp include/secrets/general_secrets.h.example include/secrets/general_secrets.h
```

All API Keys must be encrypted in `AES-128-CBC`.
`ENCRYPTION_KEY` is used to encrypt/decrypt the API keys stored in the device.

For detail on how to encrypt the API Keys, see:
https://github.com/gogo-boot/aes-demo

## Pin Configuration

### Modifying Pin Configuration

1. **Edit `include/config/pins.h`**
2. **Update GPIO assignments**

## Board Identification

Each build environment sets a board define via `-D` flag in `platformio.ini`.
The naming convention is `BOARD_{chip}_{product}`:

| Define | Chip | Product | PlatformIO Environments |
|--------|------|---------|------------------------|
| `BOARD_C3_SUPERMINI` | ESP32-C3 | ESP32-C3 Super Mini | `esp32-c3-*` |
| `BOARD_C5_XIAO` | ESP32-C5 | Seeed XIAO ESP32-C5 | *(future)* |
| `BOARD_S3_E1001` | ESP32-S3 | TRMNL E1001 PCB | `esp32-s3-e1001-*` |
| `BOARD_S3_EE04` | ESP32-S3 | TRMNL EE04 PCB | `esp32-s3-ee04-*` |

Use these defines for board-specific conditional compilation:

```cpp
#ifdef BOARD_S3_E1001
    // Code only for E1001 board (e.g., SHT4x sensor)
#endif

#if defined(BOARD_S3_E1001) || defined(BOARD_S3_EE04)
    // Code for all ESP32-S3 boards (e.g., battery, buttons)
#endif
```

Feature flags derived from board defines are set in `include/build_config.h`:

| Flag | Meaning | Enabled for |
|------|---------|-------------|
| `SHOW_BATTERY_STATUS` | Has battery ADC circuit | E1001, EE04, C5 |
| `HAS_BUTTON` | Has physical buttons | E1001, EE04, C5 |

## Building and Flashing

following command example runs with default `esp32-s3-production` environments.
you can change environment with `-e` option.

### 1. Build Project

```bash
# Build firmware (HTML config page is embedded automatically via pre-build script)
pio run
```

### 2. Upload Firmware

```bash
# Upload firmware
pio run --target upload
```

### 3. Monitor Serial Output

```bash
# Open serial monitor
pio device monitor
```

```bash
# Or with specific baud rate
pio device monitor --baud 115200
```
