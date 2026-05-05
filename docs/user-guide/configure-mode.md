# Configure Mode

Configure Mode is the only time you can change the settings on your MyStation.
During normal day-to-day use, the settings screen is not available — and this is intentional.

---

## Why Can I Only Configure in Configure Mode?

MyStation is designed to last for months on a single battery charge. To do this, it spends
most of its time in a very low-power sleep state. It wakes up briefly, fetches data, updates
the display, and immediately goes back to sleep.

Keeping the settings screen open and waiting for your input would use a lot more power.
So the settings screen is only available when you deliberately turn it on — this is
**Configure Mode**.

> 🔋 Think of it like this: Configure Mode is like a shop that only opens when you knock.
> Once you're done, it closes again to save energy.
---

## How to Enter Configure Mode

> 📷 *[Photo placeholder: device in hand, finger pressing Button 1]*

1. **Press and hold Button 1** (the left button) for **5 seconds**
2. The display refreshes and shows a setup screen with a link or QR code
3. MyStation is now broadcasting its own WiFi hotspot named **`MyStation-XXXXXXXX`**

---

## Opening the Settings Page

1. On your phone or computer, open **WiFi settings**
2. Connect to **`MyStation-XXXXXXXX`** (no password needed)
3. Open your browser and go to **`http://10.0.1.1`**
4. The MyStation settings page will appear

> 📷 *[Photo placeholder: phone browser showing the settings page]*

---

## Your Previous Settings Are Already Loaded

You don't need to start from scratch every time. When Configure Mode opens,
all your previous settings are pre-filled:

| Setting                | Remembered? |
|------------------------|-------------|
| Home WiFi network      | ✅ Yes       |
| Transport stop         | ✅ Yes       |
| Display mode           | ✅ Yes       |
| Update intervals       | ✅ Yes       |
| Sleep schedule         | ✅ Yes       |
| Weekend mode           | ✅ Yes       |
| Transport type filters | ✅ Yes       |
| Location               | ✅ Yes       |

Just change what you need and press **Save Settings**.

---

## Saving and Returning to Normal Use

Once you press **"Save Settings"**, MyStation saves everything and **restarts automatically**.
Normal operation (fetching data and updating the display) resumes within about 30 seconds.

You can also exit Configure Mode without saving by sliding the power switch to OFF, then back to ON to restart the
device. In that case, your previous settings remain unchanged.

---

## Erasing All Settings (Factory Reset)

A factory reset wipes everything: your WiFi, stop, and all configuration.
Use this only if you want to set the device up completely from scratch.

> ⚠️ **This cannot be undone.** You will need to go through the full setup again.

**How to factory reset:**

1. Press and hold **Button 1 + Button 2 at the same time** for **5 seconds**
2. The display will show a reset confirmation
3. The device restarts as if brand new — follow the [Quick Start Guide](quick-start.md)
