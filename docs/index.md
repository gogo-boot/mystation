# MyStation Documentation

Welcome to the MyStation documentation! This guide will help you get started, use, and understand your public
transport departure board.

## 📖 Documentation Sections

### For Users

If you want to build, set up, and use MyStation:

**[👋 User Guide](user-guide/index.md)** - Complete guide for end users

- [Quick Start](user-guide/quick-start.md) - Get running in 15 minutes
- [Understanding the Display](user-guide/understanding-display.md) - What information is shown
- [Button Controls](user-guide/button-controls.md) - Using physical buttons
- [Troubleshooting](user-guide/troubleshooting.md) - Common issues and solutions

### For Developers

If you want to understand the code, contribute, or modify MyStation:

**[🔧 Developer Guide](developer-guide/index.md)** - Technical documentation

- [Architecture Overview](developer-guide/index.md) - System design
- [Boot Process](developer-guide/boot-process.md) - Detailed boot flow
- [Configuration Layers](developer-guide/configuration-layers.md) - How settings are stored
- [Display System](developer-guide/display-system.md) - E-paper rendering
- [API Integration](developer-guide/api-integration.md) - External API usage
- [Hardware Assembly](developer-guide/hardware-assembly.md) - Pin configuration and wiring
- [Development Setup](developer-guide/development-setup.md) - PlatformIO environment
- [Testing](developer-guide/testing.md) - Unit tests and mocks
- [Run Book](developer-guide/run-book.md) - Operational procedures

### Quick Reference

**[📋 Reference Guides](reference/)** - Quick lookup

- [Configuration Keys](reference/configuration-keys-quick-reference.md) - All settings

## 🚀 Quick Links

### New to MyStation?

1. **[What is MyStation?](user-guide/index.md)** - Overview and features
2. **[Quick Start Guide](user-guide/quick-start.md)** - Build and configure in 15 minutes
3. **[Troubleshooting](user-guide/troubleshooting.md)** - Common issues

### Common Tasks

- **[First Time Setup](user-guide/quick-start.md)** - WiFi and station
  configuration
- **[Change WiFi Network](user-guide/quick-start.md)** - Reconfigure WiFi
- **[Understanding Display](user-guide/understanding-display.md)** - Read the screen

### Developer Tasks

- **[Development Environment](developer-guide/development-setup.md)** - Set up PlatformIO
- **[Boot Process](developer-guide/boot-process.md)** - How the system starts
- **[Hardware Assembly](developer-guide/hardware-assembly.md)** - GPIO assignments and wiring
- **[API Integration](developer-guide/api-integration.md)** - Add API keys

## 📦 What's Included

MyStation is a complete e-paper display system:

- ✅ **Real-time Transport Data** - RMV (German public transport)
- ✅ **Weather Information** - DWD (German Weather Service)
- ✅ **WiFi Configuration** - Web-based setup
- ✅ **OTA Updates** - Wireless firmware updates
- ✅ **Power Management** - Intelligent deep sleep
- ✅ **Multiple Display Modes** - Weather, departures, or both

## 🔧 Hardware Requirements

### Required

- ESP32-C3 Super Mini OR ESP32-S3 development board
- 7.5" e-paper display (GDEY075T7, 800x480)
- USB-C cable
- WiFi network (2.4 GHz - **5 GHz not supported**)

### Optional

- 3.7V LiPo battery (1000-2500mAh)
- Physical buttons (ESP32-S3 only)
- Enclosure

**See:** [Hardware Assembly](developer-guide/hardware-assembly.md) for wiring details

## ⚠️ Important Notes

### WiFi Requirements

> **MyStation only works with 2.4 GHz WiFi networks.** 5 GHz networks are not supported due to higher energy
> consumption.

### Button Support

> **Physical buttons are only available on ESP32-S3** boards. The ESP32-C3 Super Mini does not have button support
> configured.

### Regional Support

> **Public transport data is for German transit systems** (RMV API). Weather data uses German Weather Service (DWD).

## 📚 Documentation Structure

```
docs/
├── user-guide/          # End-user documentation
│   ├── index.md        # User guide overview
│   ├── quick-start.md  # Getting started
│   ├── understanding-display.md
│   ├── button-controls.md
│   └── troubleshooting.md
│
├── developer-guide/     # Developer documentation
│   ├── index.md        # Developer overview
│   ├── boot-process.md # Boot flow details
│   ├── configuration-layers.md
│   ├── display-system.md
│   ├── api-integration.md
│   ├── hardware-assembly.md
│   ├── development-setup.md
│   ├── testing.md
│   ├── testing-mocks.md
│   ├── testing-rtc.md
│   ├── github-actions.md
│   └── run-book.md
│
└── reference/           # Quick reference
    └── configuration-keys-quick-reference.md
```

## 🎯 Getting Started

### I want to use MyStation

1. Read [User Guide Overview](user-guide/index.md)
2. Follow [Quick Start Guide](user-guide/quick-start.md)
3. Refer to [Troubleshooting](user-guide/troubleshooting.md) if needed

### I want to develop MyStation

1. Read [Developer Guide](developer-guide/index.md)
2. Understand [Boot Process](developer-guide/boot-process.md)
3. Set up [Development Environment](developer-guide/development-setup.md)
4. Review [Architecture](developer-guide/index.md)

## 🆘 Getting Help

### Troubleshooting

Most issues are covered in the [Troubleshooting Guide](user-guide/troubleshooting.md):

- WiFi connection problems
- Display not updating
- Battery issues
- Button problems (ESP32-S3)

### Common Solutions

1. **Check WiFi**: Must be 2.4 GHz network
2. **Check wiring**: Verify pin connections
3. **Check serial monitor**: Error messages show here
4. **Try factory reset**: Hold Button 1 for 5 seconds during power-on

### Still Stuck?

- Check GitHub issues for similar problems
- Review serial monitor output
- Verify all prerequisites met
- Check hardware connections

## 🔗 External Resources

- [ESP32 Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [PlatformIO Docs](https://docs.platformio.org/)
- [GxEPD2 Library](https://github.com/ZinggJM/GxEPD2)
- [RMV API](https://www.rmv.de/auskunft/bin/jp/query.exe/dn)

## 📄 License

[Add your license information here]

## 🤝 Contributing

Contributions are welcome! Please:

1. Read the [Developer Guide](developer-guide/index.md)
2. Check existing issues
3. Follow code conventions
4. Write tests for new features
5. Update documentation

---

**Ready to get started?** Head to the [Quick Start Guide](user-guide/quick-start.md)!

