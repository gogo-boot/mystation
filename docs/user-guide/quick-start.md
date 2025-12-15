# Quick Start Guide

Get your MyStation up and running in 3 minutes!

## Step 1: WiFi Configuration (1 minutes)

### Connect to MyStation

1. **Find the WiFi network** named `MyStation-XXXXXXXX` (XXXXXXXX is your device's unique ID)
2. **Connect** using your phone or computer
3. **No password required** for initial setup

### Configure WiFi

1. **Open your browser** and go to `http://10.0.1.1`
2. **Select your home WiFi** network from the list
    - ⚠️ **Make sure it's a 2.4 GHz network** (5 GHz will not work)
3. **Enter your WiFi password**
4. **Click "Save"**

The device will:

- Connect to your WiFi network
- Get an IP address
- verify internet connectivity
- Start the configuration web interface
- Detect your location automatically

> 💡 **Tip**: After connecting to WiFi, the serial monitor will show your device's IP address

## Step 2: MyStation Web Configuration (2 minutes)

### Access Configuration Interface

After WiFi setup, access the configuration page:

- **By IP**: `http://192.168.1.XXX` (check display to get exact IP)
- **By mDNS**: `http://mystation.local` (if your router supports mDNS)

### Display Settings

Configure how and when to update:

- **Display Mode**: What to show on screen
    - Half & Half: Weather + Departures split screen
    - Weather Only: Full screen weather display
    - Departures Only: Full screen departure display

### Configure Your Location

The device will automatically:

1. ✅ Detect your brief location from Wifi
2. ✅ Find and display nearby public transport stops in the configuration interface

### Configure Your Town/City for Weather Forecast

1. **Review the detected location** shown on the configuration page
1. **If incorrect, enter your town/city manually**
1. **Update Interval**: How often to refresh data (default: 3 hours)

### Configure Your Transport Stop

1. **Review the list of nearby stops** shown on the configuration page
2. **Select your preferred departure station**
3. **Choose transport types** you want to see (RE, S-Bahn, Bus, etc.)
1. **Update Interval**: How often to refresh data (default: 5 minutes)

### Deep Sleep Settings

- **Sleep Schedule** (Optional): Set quiet hours
    - Example: Sleep from 23:00 to 06:00
    - Saves battery overnight

### Save Configuration

Click **"Save Settings"** and your device will:

1. Save all settings to permanent storage
1. Mystation restarts itself and up and running

## Step 5: Up and Running

### Check the Display

Your e-paper display should now show:

- 🌤️ Current weather information
- 🚌 Upcoming departures from your selected stop
- ⏰ Last update timestamp
- 🔋 Battery status (ESP32-S3 only)

### Need to Reconfigure?

Hold button 1 longer than 5 seconds. It will start over the configuration process from beginning.

## Common First-Time Issues

### "Cannot connect to WiFi network"

- ✅ Your Wifi is not required a captive portal login, like a hotel or public wifi
- ✅ Check that WiFi password is correct
- ✅ Make sure signal strength is adequate

### "No nearby stops found"

- ✅ Verify internet connection is working
- ✅ Ensure you're in an area covered by RMV (German public transport)

For more help, see the [Troubleshooting Guide](troubleshooting.md).

## Next Steps

- 📖 [Understanding the Display](understanding-display.md) - Learn what information is shown
- 🔧 [Troubleshooting](troubleshooting.md) - Common issues and solutions

Congratulations! Your MyStation is now running! 🎉

## What is MyStation?

MyStation is a battery-powered e-paper display that shows real-time public transport departures and weather information
for your location. It's designed to be:

- 🚌 **Always Up-to-Date** - Displays real-time German public transport (RMV) departures
- 🌤️ **Weather-Aware** - Shows current weather and forecasts from German Weather Service (DWD)
- 🔋 **Ultra Low Power** - Runs for 2-14 days on a single battery charge with intelligent sleep modes
- 📱 **Easy to Configure** - Simple web interface for setup and customization
- 🔒 **Privacy-Focused** - All data processing happens locally on your device
- 📡 **WiFi Connected** - Automatic updates over 2.4 GHz WiFi networks

## Need Help?

- 🚀 [Quick Start Guide](quick-start.md) - First-time setup
- 🔧 [Troubleshooting](troubleshooting.md) - Common issues and solutions
- 📖 [Full Documentation](index.md) - Complete documentation index
