# Button Controls

MyStation has **3 physical buttons** on the side of the device. This page explains what you can do with them,
organised by the task you want to accomplish.

---

## Device Buttons Overview

> 📷 *[Photo placeholder: front/side view of device with Button 1, Button 2, Button 3 labelled]*

| Button   | Location |
|----------|----------|
| Button 1 | Left     |
| Button 2 | Middle   |
| Button 3 | Right    |

---

## Use Cases — What Do You Want to Do?

### "I want to see everything at a glance"

**Press Button 1** (short press)

The display switches to **Half & Half mode**:

- Left half: current weather summary
- Right half: next departures from your stop

This reverts to your configured display mode after **2 minutes**.

---

### "Do I need an umbrella? What's the weather like?"

**Press Button 2** (short press)

The display switches to **Weather Full mode**:

- Large weather icon and description
- Current temperature and "feels like"
- 12-hour temperature forecast graph
- Wind, humidity, precipitation probability
- Sunrise and sunset times

This reverts to your configured display mode after **2 minutes**.

---

### "When is the next train / bus?"

**Press Button 3** (short press)

The display switches to **Transport Full mode**:

- Full-screen departure list
- More departures visible at once
- Line number, destination, departure time
- Delay information and platform number

This reverts to your configured display mode after **2 minutes**.

---

### "I want to change my stop, location, or settings"

**Hold Button 1 for 5 seconds**

MyStation enters **Configure Mode**:

- The display shows the configuration screen
- MyStation broadcasts a WiFi hotspot (`MyStation-XXXXXXXX`)
- Connect your phone to that hotspot and open `http://10.0.1.1`
- All your previous settings are pre-loaded — only change what you need

See [Configure Mode](configure-mode.md) for the full walkthrough.

---

### "I want to see device info and current configuration"

**Hold Button 2 for 5 seconds**

The display shows **Application Info**:

- Firmware version
- Current display mode
- Configured update intervals (weather and transport)
- WiFi status and IP address
- Battery level

---

### "I want to update the firmware manually"

**Hold Button 3 for 5 seconds**

Triggers an **OTA (Over-The-Air) firmware update** immediately:

- MyStation connects to the update server
- If a new firmware version is available, it downloads and installs it
- The device restarts after a successful update

> 💡 Normally you don't need to do this — OTA updates happen automatically overnight. Use this if you want to
> force an immediate update.

---

### "The device is stuck or behaving incorrectly"

**Turn off and on**

There is no dedicated hardware reset button on this device. Cycling the power performs a full restart.

> 💡 A power cycle is equivalent to a full restart. All saved settings are kept

---

### "I want to completely reset the device to factory settings"

**Hold Button 1 + Button 2 simultaneously for 5 seconds**

Performs a **Factory Reset**:

- Clears all saved settings (WiFi, stop, configuration)
- Device restarts as if brand new
- You will need to go through the full setup process again

> ⚠️ **This cannot be undone.** Use only when you want to start from scratch.

---

## Temporary Mode Behaviour

When you press a button to switch display modes (Button 1, 2, or 3 short press), the new mode is **temporary**:

- Duration: **2 minutes**
- After 2 minutes, the display reverts to your **configured display mode**
- The temporary mode is indicated visually on the display

This allows you to quickly check weather or departures without permanently changing your configuration.

---

## Button Reference Table

| Action               | Button       | Press Type     | Result                      |
|----------------------|--------------|----------------|-----------------------------|
| Half & Half view     | Button 1     | Short press    | Temporary mode (2 min)      |
| Weather Full view    | Button 2     | Short press    | Temporary mode (2 min)      |
| Transport Full view  | Button 3     | Short press    | Temporary mode (2 min)      |
| Enter Configure Mode | Button 1     | Hold 5 seconds | Configuration web interface |
| Show App Info        | Button 2     | Hold 5 seconds | Device info screen          |
| Trigger OTA Update   | Button 3     | Hold 5 seconds | Firmware update             |
| Factory Reset        | Button 1 + 2 | Hold 5 seconds | Erase all settings          |
