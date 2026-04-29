# Quick Start Guide

Get your MyStation up and running in about 5 minutes!

> 📷 *[Photo placeholder: device powered on, showing initial screen]*

---

## What You Need Before You Start

- Your **home WiFi name (SSID)** and **password**
- A phone or computer with a browser
- Your WiFi must be **2.4 GHz** — 5 GHz is **not supported**
- Your WiFi must be a standard home network (no captive portal login like hotel WiFi)

---

## Step 1: Power On the Device

Turn on the device or Connect USB power. The device will boot and after ~30 seconds display a setup screen.

> 📷 *[Photo placeholder: device showing "MyStation-XXXXXXXX" WiFi hotspot name on screen]*

---

## Step 2: Connect to MyStation WiFi (1 minute)

1. On your phone or computer, open **WiFi settings**
2. Find the network named **`MyStation-XXXXXXXX`** (XXXXXXXX is your device's unique ID)
3. Connect to it — **no password required**

> 📷 *[Photo placeholder: phone WiFi list showing MyStation-XXXXXXXX network]*

---

## Step 3: Open the Configuration Page

1. Open your browser and go to **`http://10.0.1.1`**
2. The MyStation configuration page will load

> 📷 *[Photo placeholder: browser showing the MyStation configuration web page]*

---

## Step 4: Configure WiFi

1. Select your **home WiFi network** from the list
    - ⚠️ Make sure it's a **2.4 GHz** network
2. Enter your **WiFi password**
3. Click **"Connect"**

MyStation will:

- Connect to your home WiFi
- Detect your approximate location automatically
- Discover nearby public transport stops

> 💡 After connecting to your home WiFi, the configuration page will reload automatically at the device's new IP address.

---

## Step 5: Configure the Application (2 minutes)

The configuration page now shows all settings. Your location and nearby stops are already detected.

### Display Mode

Choose what to show on the screen:

- **Half & Half** — Weather on the left, departures on the right
- **Weather Full** — Full screen detailed weather
- **Transport Full** — Full screen departure board

### Location / Weather

- Review the **auto-detected location** shown on the page
- If it looks correct, leave it as-is — this gives the most accurate weather for your location
- If incorrect, enter your **city or town name** manually

> 💡 Auto-detected coordinates are more accurate than a city name. A city name uses the geographic centre of the
> city, which may be several kilometres from your actual location.
> See [Configuration Guide](configuration.md#location-settings).

### Transport Stop

1. Review the list of **nearby stops** — they are sorted by distance
2. Select your preferred departure stop
3. Choose which **transport types** to show (RE, S-Bahn, Bus, Tram, etc.)

### Update Intervals

- **Weather interval**: How often weather data refreshes (default: 3 hours)
- **Transport interval**: How often departure data refreshes (default: 5 minutes)

### Sleep Schedule (Recommended)

Set quiet hours to save battery overnight. Example: sleep from `22:30` to `05:30`.

### Save

Click **"Save Settings"**. MyStation saves all settings and **restarts automatically**.

---

## Step 6: Up and Running 🎉

After restarting, MyStation will fetch data and update the display. You should see:

- 🌤️ Current weather information
- 🚌 Upcoming departures from your selected stop
- ⏰ Last update time in the footer
- 🔋 Battery status

> 📷 *[Photo placeholder: device showing live weather and departure data]*

---

## Need to Reconfigure?

Hold **Button 1 for 5 seconds** to enter Configure Mode at any time.
All your previous settings will be pre-loaded. See [Configure Mode](configure-mode.md).

---

## Common First-Time Issues

### "Cannot connect to MyStation WiFi"

- Wait 30–60 seconds after powering on — the device needs time to boot
- Make sure your phone is not locked to 5 GHz only

### "WiFi connection failed"

- ✅ Check that your WiFi password is correct (case-sensitive)
- ✅ Verify your network is 2.4 GHz
- ✅ Your network must not require a browser login (captive portal)

### "No nearby stops found"

- ✅ Verify internet connection is working
- ✅ You must be in an area covered by RMV (German public transport network)

For more help, see the [Troubleshooting Guide](troubleshooting.md).
