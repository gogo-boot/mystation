# Troubleshooting Guide

This guide helps you solve common issues with your MyStation e-paper departure board.

## WiFi & Network Issues

### Cannot Connect to MyStation WiFi Access Point

**Symptoms:**

- `MyStation-XXXXXXXX` network not visible
- Can't join configuration WiFi

**Solutions:**

1. **Wait for startup** (30-60 seconds after power on)
    - Device needs time to boot

1. **Check WiFi band**
    - ⚠️ MyStation only works with **2.4 GHz WiFi**
    - If your phone is set to "5 GHz only", change to "Auto" or "2.4 GHz"
    - Some phones hide 2.4 GHz networks by default

1. **Try different device**
    - Use different phone/computer
    - Some devices have WiFi compatibility issues

1. **Charge Battery**
    - Low battery may prevent WiFi AP from starting
    - Connect USB power or charge battery

### Cannot Connect MyStation to Home WiFi

**Symptoms:**

- Configuration page shows "Failed to connect to WiFi"
- Device keeps restarting
- Can't complete WiFi setup

**Solutions:**

1. **Verify WiFi network is 2.4 GHz**
    - ⚠️ **Critical**: MyStation only supports 2.4 GHz networks
    - 5 GHz networks are **not supported** (high energy consumption)
    - Check your router settings
    - Many routers have separate 2.4 GHz and 5 GHz SSIDs
    - Look for network names like "MyNetwork-2.4G" or "MyNetwork"

2. **Check WiFi password**
    - Verify password is correct (case-sensitive)
    - Watch for special characters
    - Try typing password in notes app first, then copy/paste

3. **Check WiFi signal strength**
    - Move device closer to router
    - Ensure no thick walls or metal objects blocking signal
    - Signal strength should be at least -70 dBm

4. **Router compatibility**
    - supports: WPA/WPA2 Personal
    - NOT supported: WPA3, WPA2 Enterprise
    - Check router security settings

5. **Disable WiFi features**
    - Turn off AP Isolation
    - Disable Client Isolation
    - Enable DHCP
    - Allow new device connections

6. **Check MAC address filtering**
    - If enabled on router, add MyStation's MAC address
    - MAC address shown in serial monitor during boot

### Device Connects but Can't Access Internet

**Symptoms:**

- WiFi shows connected
- Display doesn't update
- "Failed to fetch data" errors

**Solutions:**

1. **Check internet connection**
    - Verify router has internet access
    - Test with other devices

2. **Check firewall settings**
    - Router firewall might block HTTPS requests
    - Whitelist MyStation's IP
    - Allow outbound connections on ports 80, 443

3. **DNS issues**
    - Router might have DNS problems
    - Check serial monitor for DNS resolution errors

### mDNS Not Working (Can't Access mystation.local)

**Symptoms:**

- `http://mystation.local` doesn't work
- "Server not found" error

**Solutions:**

1. **Use IP address instead**
    - Check serial monitor for device IP
    - Use `http://192.168.1.XXX` directly

1. **Network configuration**
    - mDNS requires multicast support
    - Some routers block mDNS
    - Try on different network

---

## Display Issues

### Display Not Updating

**Symptoms:**

- E-paper shows old data
- Display never refreshes
- Stuck on initial screen

**Solutions:**

1. **Push Reset button**
1. **Check WiFi connection**
    - Verify device connects to 2.4 GHz WiFi
1. **Charge Battery**

### Display Has "Ghost" Images

**Symptoms:**

- Previous image still faintly visible
- Overlapping images

**Solutions:**

1. **This is normal for e-paper**
    - E-paper displays can show ghosting
    - Periodic full refresh reduces this

2. **Enable/verify full refresh**
    - Code should do full refresh periodically
    - Full refresh clears ghosts

3. **Display wearing out**
    - E-paper has limited refresh cycles
    - Ghosting increases with age
    - ~10,000-100,000 full refreshes typical

---

## Power & Battery Issues

### Battery Drains Too Quickly

**Solutions:**

1. **Increase update interval**
    - Change from 5 to 10 or 15 minutes
    - Doubles/triples battery life
    - Still provides current information

3. **Enable sleep schedule**
    - Configure quiet hours (e.g., 23:00-06:00)
    - Save power overnight
    - See web configuration interface

3. **Check WiFi strength**
    - Weak signal uses more power
    - Move closer to router
    - Consider WiFi range extender

4. **Reduce display complexity**
    - Weather-only mode uses less data
    - Fewer API calls = less power

6. **Battery health**
    - Replace if battery damaged/old

### Device Won't Power On

**Symptoms:**

- No LED when plugged in
- Completely dead
- Won't charge

**Solutions:**

1. **Try different USB cable**
    - Some cables are charge-only (no data)
    - Try known-good cable

2. **Try different power source**
    - Different USB port
    - Different charger
    - Computer USB vs wall adapter

4. **Disconnect battery**
    - Test with USB power only
    - Battery fault might prevent boot

6. **Hardware failure**
    - Board may be damaged
    - Replace or repair

---

### Factory Reset Button Not Working

**Symptoms:**

- Hold Button 1 for 5 seconds, nothing happens
- No factory reset occurs

**Solutions:**

1. **Timing is critical**
    - Continue holding for full 5 seconds
    - Count slowly: 1-Mississippi, 2-Mississippi...

---

## Configuration Issues

### Can't Access Configuration Page

**Symptoms:**

- Can't load `http://10.0.1.1` or `http://mystation.local`
- Page won't load
- Connection timeout

**Solutions:**

1. **Verify connected to correct WiFi**
    - In config mode: Connect to `MyStation-XXXXXXXX`
    - In normal mode: Same network as MyStation

2. **Check device mode**
    - Config mode: Use `http://10.0.1.1`
    - Normal mode: Use device IP or `http://mystation.local`

3. **Clear browser cache**
    - Hard refresh: Ctrl+Shift+R (PC) or Cmd+Shift+R (Mac)
    - Try different browser
    - Try incognito/private mode

### No Nearby Stops Found

**Symptoms:**

- Configuration page shows "No stops found"
- Can't select transport station

**Solutions:**

1. **Verify location**
    - MyStation uses Google Geolocation
    - Verify internet connection

2. **Check RMV coverage**
    - RMV covers German public transport
    - Must be in Germany or covered area
    - Try manual location entry

### Settings Don't Save

**Symptoms:**

- Change settings, but they reset
- Configuration reverts after restart

**Solutions:**

1. **Click "Save" button**
    - Must explicitly save
    - Look for confirmation message

2. **NVS storage full**
    - Rare, but possible
    - Try factory reset
    - Clears old data

3. **NVS write errors**
    - Check serial monitor for errors
    - May indicate flash memory issue

4. **Power loss during save**
    - Don't disconnect power while saving
    - Wait for confirmation message

---

## Data & API Issues

### Weather Data Not Showing

**Symptoms:**

- Display shows "---" for weather
- Temperature missing
- Weather icon blank

**Solutions:**

1. **Check location configured**
    - Weather requires valid coordinates
    - Verify in configuration page

2. **DWD API access**
    - Check internet connection
    - Verify DWD service online
    - Serial monitor shows API errors

3. **Location outside Germany**
    - DWD covers Germany only
    - Won't work in other countries
    - Consider alternative weather API

4. **Wait for next update**
    - Data might be loading
    - Check update interval setting
    - Monitor serial output

### Transport Departures Not Showing

**Symptoms:**

- No departures listed
- Empty departure board
- Only weather shows

**Solutions:**

1. **Check station configured**
    - Must select transport stop
    - Verify in configuration page

2. **RMV API access**
    - Check internet connection
    - Verify RMV API key
    - Check API limits not exceeded

3. **No departures available**
    - Late night/early morning
    - Check if service actually running
    - Try different time of day

4. **Transport filter too restrictive**
    - Check which transport types enabled
    - Enable more types (Bus, Tram, S-Bahn, etc.)

---

### Check Documentation

- 📖 [User Guide](index.md) - Complete user documentation
- 📚 [Understanding Display](understanding-display.md) - Display modes and troubleshooting

### Report Issues

If you found a bug:

- Check GitHub issues for similar problems
- Create new issue with details
- Include serial monitor output
- Describe steps to reproduce

---

**Remember**: Most issues are related to WiFi (2.4 GHz only!), wiring, or configuration. Start with the basics and work
through systematically.
