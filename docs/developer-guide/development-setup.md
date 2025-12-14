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

I recommend using the `TRMNL 7.5" (OG) DIY Kit (SKU 104991005)` from Seeed Studio, which includes the e-Paper display,
ESP32-S3 Plus, E-Paper Display, Display Hat, and a rechargeable battery with BMS.
It supports also charging Battery via USB-C. No need to buy charging circuit separately.
You can also measure the voltage of the battery and get current battery level.
You don't need to solder anything, just assemble the parts.

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

All API Kyes must be encrypted in `AES-128-CBC`.
`ENCRYPTION_KEY` is used to encrypt/decrypt the API keys stored in the device.

For detail on how to encrypt the API Keys, see:
https://github.com/gogo-boot/aes-demo

## Building and Flashing

following command example runs with default `esp32-s3-production` environments.
you can change environment with `-e` option.

### 1. Build Project

```bash
# Build filesystem (for web based configuration )
pio run --target buildfs
```

```bash
# Build firmware
pio run
```

### 2. Upload Firmware

```bash
# The `data/` directory contains files uploaded to the ESP32's filesystem
# Upload filesystem
pio run --target uploadfs
```

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

## Next Steps

After software setup:

- [API Keys Setup](./api-keys.md) - Configure external services
- [Quick Start Guide](./quick-start.md) - First run configuration
- [Development Guide](./development.md) - Contributing to the project
