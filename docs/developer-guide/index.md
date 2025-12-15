# Developer Guide

Welcome to the MyStation developer documentation. This guide provides technical details about the system architecture,
implementation, and development processes.

## For Developers

This section is for developers who want to:

- 🔧 Understand the system architecture
- 📝 Contribute to the project
- 🐛 Debug and fix issues
- ✨ Add new features
- 🔌 Modify hardware configuration

> 👤 **Are you not a developer?** See the [User Guide](../user-guide/index.md) instead.

## Project Overview

MyStation is an ESP32-based e-paper display system that shows real-time public transport departures and weather
information.

### Key Technologies

- **Platform**: ESP32-C3 / ESP32-S3
- **Framework**: Arduino framework via PlatformIO
- **Display**: 7.5" e-paper (GDEY075T7, 800x480)
- **Network**: WiFi 2.4 GHz (802.11 b/g/n)
- **Storage**: NVS (Non-Volatile Storage) for configuration
- **OTA**: Over-the-air firmware updates
- **Power**: Deep sleep with intelligent wake-up

### Core Features

1. **Configuration System** - Multi-phase boot with web-based setup
2. **WiFi Management** - Auto-connect, fallback, mDNS
3. **Display System** - Multiple modes, efficient e-paper rendering
4. **Power Management** - Deep sleep, battery monitoring, scheduled wake
5. **OTA Updates** - Secure firmware updates over WiFi
6. **API Integration** - Google Geolocation, RMV API, DWD Weather, OpenStreetMap Nominatim

## Quick Navigation

### 🏗️ Architecture & Design

- [**Boot Process**](boot-process.md) - Detailed boot flow and phases

### ⚙️ Core Systems

- [**Configuration Layers**](configuration-layers.md) - NVS storage, phases, data structures
- [**Display System**](display-system.md) - E-paper rendering, modes, layouts
- [**Power Management**](run-book.md) - Deep sleep, wake sources, battery (see Run Book)
- [**OTA System**](run-book.md) - Firmware updates, scheduling, rollback (see Run Book)

### 📡 Integration & APIs

- [**API Integration**](api-integration.md) - Google, RMV, DWD APIs
- [**WiFi Management**](run-book.md) - Connection, fallback, mDNS (see Run Book)

### 🛠️ Development

- [**Development Setup**](development-setup.md) - Getting started with PlatformIO
- [**Testing**](testing.md) - Native tests, mocks, debugging
- [**Run Book**](run-book.md) - Operational procedures and troubleshooting

### 🔌 Hardware

- [**Hardware Assembly**](hardware-assembly.md) - Pin connections and wiring diagrams

## Technology Stack

### Hardware

| Component | Model         | Purpose                 |
|-----------|---------------|-------------------------|
| MCU       | ESP32-C3 / S3 | Main processor          |
| Display   | GDEY075T7     | 7.5" e-paper, 800x480   |
| WiFi      | Built-in      | 2.4 GHz 802.11 b/g/n    |
| Storage   | 4MB Flash     | Firmware + SPIFFS + NVS |
| Power     | 3.7V LiPo     | Battery powered         |

### Software

| Library       | Version | Purpose            |
|---------------|---------|--------------------|
| PlatformIO    | Latest  | Build system       |
| Arduino-ESP32 | 6.5.0   | Framework          |
| WiFiManager   | 2.0.17  | WiFi configuration |
| ArduinoJson   | 6.21.4  | JSON parsing       |
| GxEPD2        | 1.6.4   | E-paper driver     |
| U8g2          | 1.8.0   | Font rendering     |

### APIs

| Service            | Purpose               | Authentication      |
|--------------------|-----------------------|---------------------|
| Google Geolocation | Location detection    | API Key             |
| RMV API            | Public transport data | API Key             |
| DWD Weather        | Weather forecasts     | None (open)         |
| GitHub             | OTA updates           | Certificate pinning |

## Key Concepts

### Configuration Phases

MyStation uses a three-phase boot process:

1. **Phase 1: WiFi Setup** - No WiFi configured, create AP
2. **Phase 2: App Setup** - WiFi configured, need station config
3. **Phase 3: Operational** - Fully configured, normal operation

See [Boot Process](boot-process.md) for details.

### Deep Sleep Cycle

Normal operation follows this pattern:

```
Wake Up → Connect WiFi → Fetch Data → Update Display → Deep Sleep
  ↑                                                          │
  └──────────────────────────────────────────────────────────┘
                    (Configured interval)
```

### Display Modes

Three display modes supported:

- **DISPLAY_MODE_HALF_AND_HALF** (0) - Weather + Departures
- **DISPLAY_MODE_WEATHER_ONLY** (1) - Weather full screen
- **DISPLAY_MODE_DEPARTURE_ONLY** (2) - Departures full screen

See [Display System](display-system.md) for rendering details.

### Configuration Storage

All settings stored in NVS (Non-Volatile Storage):

- WiFi credentials (encrypted namespace)
- Station configuration
- Display preferences
- Update intervals
- Sleep schedule

See [Configuration Layers](configuration-layers.md) for data structures.

## Code Organization

```
mystation/
├── src/                 # Source code
│   ├── main.cpp        # Entry point
│   ├── api/            # API clients
│   ├── config/         # Configuration
│   ├── display/        # Display rendering
│   ├── ota/            # OTA updates
│   ├── sec/            # Security (AES)
│   └── util/           # Utilities
├── include/            # Header files (mirrors src/)
├── lib/                # Libraries
├── test/               # Unit tests
├── data/               # SPIFFS files
├── docs/               # Documentation
└── platformio.ini      # Build configuration
```

## Common Development Tasks

### Testing Changes

```bash
# Run native tests
pio test -e native

# Run specific test
pio test -e native -f test_timing_manager

# Upload and monitor
pio run --target upload && pio device monitor
```

See [Testing](testing.md) for test framework details.

## Contributing

### Code Style

- **Indentation**: 4 spaces (no tabs)
- **Braces**: K&R style (opening brace on same line)
- **Naming**:
    - Classes: PascalCase
    - Functions: camelCase
    - Constants: UPPER_SNAKE_CASE
    - Variables: camelCase

**refer .clang-format file for full style guide**

### Git Workflow

1. Fork the repository
2. Create feature branch
3. Make commits (atomic, well-described)
4. Test thoroughly
5. Submit pull request
6. Address review comments

### Documentation

- Update relevant documentation with code changes
- Add examples for new features
- Keep README.md current
- Document breaking changes

## Resources

### Documentation

- 📖 [User Guide](../user-guide/index.md) - End-user documentation
- 📚 [Configuration Keys Quick Reference](../reference/configuration-keys-quick-reference.md) - Configuration keys

### External Resources

- [ESP32 Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [PlatformIO Docs](https://docs.platformio.org/)
- [GxEPD2 Library](https://github.com/ZinggJM/GxEPD2)
- [Arduino Reference](https://www.arduino.cc/reference/en/)

### Community

- GitHub Issues - Bug reports and feature requests
- Pull Requests - Code contributions
- Discussions - Questions and ideas

## Next Steps

### New to the Project?

1. Understand [Boot Process](boot-process.md)
1. Set up [Development Environment](development-setup.md)

### Ready to Code?

1. Check open issues for tasks
2. Set up your development environment
3. Start with small improvements
4. Ask questions if stuck

### Want to Contribute?

1. Read contributing guidelines
2. Pick an issue or suggest a feature
3. Discuss approach before major changes
4. Write tests for new functionality
5. Update documentation
6. Submit pull request

---

**Questions?** Check the [Troubleshooting Guide](../user-guide/troubleshooting.md) or open an issue on GitHub.
