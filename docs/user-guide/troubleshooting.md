# Troubleshooting Guide

Something not working? This guide walks through the most common issues and how to fix them.

---

## WiFi & Network Issues

### The MyStation WiFi Hotspot Does Not Appear

**Symptoms:** You can't find `MyStation-XXXXXXXX` in your phone's WiFi list.

**Solutions:**

1. **Wait a bit longer after powering on** — the device needs up to 60 seconds to start up
2. **Check that the device has power** — if the battery is low the device may not start up. Charge it via USB and try
   again
3. **Try a different phone or computer** — some devices have trouble seeing certain WiFi networks
4. **Make sure you are looking for a 2.4 GHz network** — if your phone is set to "5 GHz only", change it to "automatic"

---

### MyStation Won't Connect to Your Home WiFi

**Symptoms:** After entering your WiFi details, the device keeps failing to connect.

**Solutions:**

1. **Check your WiFi password** — it is case-sensitive, so `Password1` is different from `password1`
2. **Make sure it's a 2.4 GHz network** — 5 GHz is not supported. Look for the 2.4 GHz version of your network name
3. **Check your signal strength** — move the device closer to the router and try again
4. **Check your router type** — MyStation works with standard home routers (WPA/WPA2). It does not work with hotel WiFi,
   public WiFi with login screens, or business networks with special security

---

### Connected but Can't Reach the Settings Page

**Symptoms:** The device is connected to WiFi, but `http://mystation.local` or the IP address won't open.

**Solutions:**

1. **Make sure your phone is on home WiFi, not mobile data** — switch to WiFi first
2. **Try the IP address directly** — check the display footer for the device's current IP address and open that in your
   browser (e.g. `http://192.168.1.42`)
3. **Try a different browser** — some browsers block local addresses
4. **Your router may have device isolation turned on** — this prevents devices on the same network from talking to each
   other. Check your router settings or contact your internet provider

---

## Display Issues

### Display Is Not Updating

**Symptoms:** The screen shows old data and doesn't refresh.

**Solutions:**

1. **Restart the device** — slide the power switch to OFF, then back to ON. This device has an embedded battery; the
   power switch is the correct way to restart it.
2. **Check WiFi** — make sure the device is still connected to your home WiFi (2.4 GHz)
3. **Check battery level** — low battery can cause updates to stop. Charge the device via USB

---

### Display Shows "Ghost" Images

**Symptoms:** A faint shadow of a previous image is still visible on screen.

**Solutions:**

1. **This is normal for e-paper screens** — the display technology can leave faint traces that clear up over time
2. The device does a full screen refresh periodically to clear these ghosts

---

## Power & Battery

### Battery Drains Too Quickly

**Solutions:**

1. **Increase the update interval** — change from every 5 minutes to every 10 or 15 minutes for transport, and from 1
   hour to 3 hours for weather
2. **Set a sleep schedule** — configure quiet hours (e.g. `22:30` to `05:30`) so the device doesn't update overnight
3. **Move closer to your router** — a weak WiFi signal uses significantly more power
4. **Check battery health** — older batteries hold less charge. Replace if needed

---

### Device Won't Turn On

**Solutions:**

1. **Charge the battery** — connect the device to a USB charger and wait at least 30 minutes. The battery is built-in
   and cannot be removed or replaced easily.
2. **Try a different USB cable or charger** — some cables or chargers do not deliver enough power. Try a known-working
   cable and a wall adapter (not just a computer USB port).
3. **Slide the power switch to ON** — after charging, slide the power switch to the ON position to turn the device on

---

## Configuration Issues

### Configured Display Mode Is Not Applied

**Symptoms:** You selected "Weather Full" or "Transport Full" in the settings, but the device
always shows Half & Half after restarting.

**Solutions:**

1. **Re-enter Configure Mode and save again**
    - Hold Button 1 for 5 seconds
    - Check that the correct display mode is selected
    - Press "Save Settings"
2. **Check for a software update** — hold Button 3 for 5 seconds to trigger an update.
   This issue is fixed in recent software versions.

---

### Display Shows Wrong Time (1 Hour Off)

**Symptoms:** The time shown in the display footer is 1 hour ahead of the real time.
This tends to happen after the device has been running for several update cycles.

**Solutions:**

1. **Restart the device** — slide the power switch to OFF, then back to ON. The device re-syncs the time on each
   restart. This is a known issue that can appear around the daylight saving time changes in March and October.

---

### Settings Don't Save / Keep Resetting

**Solutions:**

1. **Make sure you clicked "Save Settings"** — look for a confirmation message after clicking
2. **Don't turn off during saving** — wait for the confirmation before disconnecting power
3. **Try a factory reset** — if settings never stick, the internal storage may need to be cleared.
   Hold Button 1 + Button 2 for 5 seconds to factory reset, then set up from scratch

---

## Data Issues

### Weather Not Showing

**Symptoms:** The weather area shows `---` or no information at all.

**Solutions:**

1. **Check your location is set correctly** — open Configure Mode and verify the location looks right
2. **Check your internet connection** — verify your router has internet access by checking with another device
3. **Note:** MyStation uses the German Weather Service (DWD), which covers Germany only. It will not show weather for
   locations outside Germany

---

### Transport Departures Not Showing

**Symptoms:** The departure area is empty, or shows no entries.

**Solutions:**

1. **Check that a stop is selected** — open Configure Mode and verify a transport stop is chosen
2. **Check the time** — very early in the morning or late at night, there may genuinely be no departures
3. **Check transport type filters** — if you have filtered out all transport types, nothing will show. Enable at least
   one type (Bus, S-Bahn, etc.)
4. **Check internet connection** — departures require a working internet connection

---

### Software Update Failed

**Symptoms:** The device restarted but the software version didn't change.

**Solutions:**

1. **Trigger a manual update** — hold Button 3 for 5 seconds to force an update check
2. **Check internet connection** — the update requires a working internet connection
3. **Try again the next night** — the device retries automatically

> 💡 **Why updates matter:** The services that provide weather and transport data occasionally
> change how they format their data. A software update is needed to keep MyStation compatible.
> If updates are disabled and the data format changes, weather or departure info may stop appearing.

---

## Factory Reset

Use this when the device is completely misconfigured, or you want to start from scratch.

> ⚠️ **All settings will be permanently erased.** You will need to set up the device again from the beginning.

**Steps:**

1. Press and hold **Button 1 + Button 2 simultaneously** for **5 seconds**
2. Confirm the reset on the display
3. Follow the [Quick Start Guide](quick-start.md) to set up again
