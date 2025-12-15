# MyStation User Guide

Welcome to MyStation, your personal weather and public transport departure board!

## What is MyStation?

MyStation is a battery-powered e-paper display that shows real-time public transport departures and weather information
for your location. It's designed to be:

- 🚌 **Always Up-to-Date** - Displays real-time German public transport (RMV) departures
- 🌤️ **Weather-Aware** - Shows current weather and forecasts from German Weather Service (DWD)
- 🔋 **Ultra Low Power** - Runs for months on a single battery charge
- 📱 **Easy to Configure** - Simple web interface for setup
- 🔒 **Privacy-Focused** - All data processing happens locally on your device
- 📡 **WiFi Connected** - Automatic updates over 2.4 GHz WiFi networks

## Key Features

### Near Real-Time Departure Board

- Display departures from your favorite public transport stops
- Filter by transport type (RE, S-Bahn, Bus, Tram, etc.)
- See departure times, delays, and platform information
- Multiple display modes: weather only, departures only, or combined

### Weather Information

- Current weather conditions
- Temperature and "feels like" temperature
- Weather icons and descriptions
- 24-hour forecast graphs

### Smart Power Management

- Intelligent deep sleep between updates
- Configurable update intervals (Weather: hours / Transport: minutes)
- Optional sleep schedule (e.g., sleep overnight)
- Battery monitoring and status display

### Easy Configuration

- WiFi access point for initial setup
- No need to install an extra application. Use Mobile-friendly web interface
- Automatic device location detection
- Automatic Nearby stop discovery
- Over-the-air (OTA) firmware updates

### Network Requirements

- WiFi network (2.4 GHz - **5 GHz is not supported**)
- Internet connection for data updates

> ⚠️ **Important**: MyStation only works with **2.4 GHz WiFi networks**. 5 GHz networks are not supported due to higher
> energy consumption.

## Device Controls

Your MyStation device includes:

- **Physical Buttons** (3 buttons) - Switch between display modes instantly
    - Button 1: Weather + Departures view
    - Button 2: Weather only view
    - Button 3: Departures only view
    - Button 1 (long press): Factory reset
- **Reset Button** - Restart the device if needed

See [Button Controls](button-controls.md) for detailed usage.

## Getting Started

### Setup & Configuration

- **[Quick Start](quick-start.md)** - Get up and running in 2 minutes

### Daily Usage

- **[Understanding the Display](understanding-display.md)** - What information is shown and where
- **[Button Controls](button-controls.md)** - Using the physical buttons

### Maintenance & Support

- **[Factory Reset](factory-reset.md)** - I want to change configuration, Change Wifi, Moved home, Reset to default
  settings

## Important Notes

### WiFi Requirements

> **MyStation only works with 2.4 GHz WiFi networks.** 5 GHz networks are not supported due to higher energy consumption
> requirements.

When setting up WiFi:

- ✅ Look for your WiFi network name (SSID)
- ✅ Make sure it's a 2.4 GHz network
- ✅ Check signal strength is adequate
- ✅ Have your WiFi password ready

### Regional Support

> **Public transport data is for German transit systems** (RMV API). Weather data uses German Weather Service (DWD).

MyStation is optimized for use in Germany with access to:

- German public transport networks (via RMV)
- German weather forecasts (via DWD)

### Battery Life

It depends on your refresh intervals and usage patterns. It varies based on user pattern.
30 Updates per day can last 3 months on a full charge.

For longer battery life:

- Increase update interval
- Enable sleep schedule (no updates overnight)
- Ensure strong WiFi signal

## Use case of Mystation

- just want to do 1 hour outdoor activities, when it is the best slot just for 1 hour?
- do i need an umbrella today?
- when is the next train to work?
- Kid wants to know if it's sunny outside before going out to play?
- Kid can decide their own if they need a jacket, umbrella before going to school?
- Do i need a sunglasses or sunblock today?
- My phone is dead, need to know if i can go outside without checking weather app?

## Security and Privacy

Mystation doesn't have microphone or camera.

It does store your WiFi credentials securely in encrypted storage on the device.
It gets automatically its Geo Location via Wifi Networks.
The found Geo Location is not exact geo location, It is brief geolocation.
The Geolocation is only used to configure device easily to avoid type in longitude and latitude manually.

It connects to your WiFi to fetch weather and transport data repeatly based on your configured intervals.
Eventually, it connects to internet to check for OTA updates.
It does not share any data with third parties beyond these API requests.

## Need Help?

- 🚀 [Quick Start Guide](quick-start.md) - First-time setup
- 🔧 [Troubleshooting](troubleshooting.md) - Common issues and solutions
- 🔄 [Factory Reset](factory-reset.md) - Reset to defaults
- 📖 [Full Documentation](../index.md) - Complete documentation index

## Quick Links

### First Time Setup

1. [Quick Start Guide](quick-start.md) - Complete setup walkthrough
2. [WiFi Configuration](quick-start.md#step-3-wifi-configuration-3-minutes) - Connect to your network
3. [Station Configuration](quick-start.md#step-4-station-configuration-5-minutes) - Select your transport stop

---

**Ready to get started?** Head to the [Quick Start Guide](quick-start.md)!

