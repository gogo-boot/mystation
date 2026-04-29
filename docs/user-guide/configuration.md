# Configuration Guide

This page explains every setting available in the MyStation configuration web interface.
You access this page by entering **Configure Mode** (hold Button 1 for 5 seconds).

See [Network Setup](network-setup.md) for how to connect to the configuration page.

---

## Display Mode

Controls what is shown on the e-paper display during normal operation.

| Mode               | What is displayed                                        |
|--------------------|----------------------------------------------------------|
| **Half & Half**    | Left half: weather summary · Right half: departure list  |
| **Weather Full**   | Full screen detailed weather with 12-hour forecast graph |
| **Transport Full** | Full screen departure board with more departure entries  |

> 💡 The display mode you configure here is your **permanent mode**. You can temporarily switch modes using the
> physical buttons (reverts after 2 minutes). See [Button Controls](button-controls.md).

---

## Location Settings

### City / Town for Weather

MyStation uses the city name to get weather data from the **German Weather Service (DWD)**. When you enter a city
name, the weather coordinates are set to the **geographic centre of that city**.

### Auto-Detect Coordinates (Recommended)

When you open the configuration page, MyStation automatically detects your **approximate location using nearby WiFi
networks** (via Google Geolocation API). This gives a coordinate that is **near your actual location**, not just
the centre of your city.

> 💡 **Why auto-detect is more accurate:**
> If you live in a suburb or on the edge of a city, the city-centre coordinate may be several kilometres away.
> Auto-detected coordinates are typically within a few hundred metres of your actual location, which gives you
> a more accurate local weather forecast — especially for precipitation and temperature.

**Recommendation**: Use the auto-detected coordinates when available. Only enter a city name manually if the
auto-detection fails or gives a wrong location.

---

## Weather Settings

### Weather Update Interval

How often MyStation wakes up to refresh weather data.

| Interval | Description                              |
|----------|------------------------------------------|
| 1 hour   | Most up-to-date, higher battery usage    |
| 2 hours  | Balanced                                 |
| 3 hours  | Default                                  |
| 6 hours  | Low battery usage, less frequent updates |
| 12 hours | Minimum updates, best battery life       |

> 🔋 Longer intervals significantly extend battery life. Weather changes slowly — a 3-hour interval is usually
> sufficient for daily planning.

---

## Transport Settings

### Transport Stop

Select which public transport stop to display departures from. MyStation automatically discovers nearby stops
based on your detected location.

1. The configuration page shows a list of **nearby stops** with distance
2. Select your preferred stop
3. If your stop is not listed, check your location settings

### Transport Update Interval

How often MyStation wakes up to fetch new departure data.

| Interval   | Description                                        |
|------------|----------------------------------------------------|
| 1 minute   | Near real-time, significantly higher battery usage |
| 3 minutes  | Very frequent                                      |
| 5 minutes  | Default                                            |
| 10 minutes | Moderate battery savings                           |
| 15 minutes | Low battery usage                                  |
| 30 minutes | Minimum updates                                    |

> 🔋 Transport data changes more frequently than weather. A 5-minute interval gives a good balance between
> freshness and battery life.

### Transport Active Hours

You can limit transport updates to a time window — for example, only during your commute hours.
Outside this window, transport updates are paused to save battery.

| Setting      | Example |
|--------------|---------|
| Active Start | `06:00` |
| Active End   | `09:00` |

### Walking Time to Stop

Enter how many minutes it takes you to walk to your stop. Departures that are too soon to catch
(given your walking time) will be shown differently or filtered.

### Transport Type Filters

Choose which types of transport to display:

| Filter | Transport type                     |
|--------|------------------------------------|
| RE     | Regional Express / Regional trains |
| S-Bahn | Suburban railway                   |
| U-Bahn | Underground / Metro                |
| Tram   | Straßenbahn                        |
| Bus    | Regular bus                        |

Enable only the types that serve your stop to reduce irrelevant entries.

---

## Sleep Schedule

Configure quiet hours during which MyStation does not fetch data or update the display.
This is the single most effective way to extend battery life.

| Setting     | Example |
|-------------|---------|
| Sleep Start | `22:30` |
| Sleep End   | `05:30` |

During sleep hours, the device enters deep sleep and does **not** wake up for data updates.
The last displayed content remains on the e-paper screen (e-paper holds its image without power).

---

## Weekend Mode

Enable different settings for weekends (Saturday and Sunday):

| Setting                 | Example |
|-------------------------|---------|
| Weekend Transport Start | `08:00` |
| Weekend Transport End   | `20:00` |
| Weekend Sleep Start     | `23:00` |
| Weekend Sleep End       | `07:00` |

---

## OTA Firmware Updates

MyStation can update its own firmware automatically over WiFi (**Over-The-Air update**).

| Setting        | Default                                      |
|----------------|----------------------------------------------|
| OTA Enabled    | ✅ Yes                                        |
| OTA Check Time | Randomised between 01:00–04:59 on first boot |

### Why OTA Updates Are Important

The weather and transport data providers (**DWD** and **RMV**) may change their API formats over time.
When this happens, the firmware must be updated to continue working correctly with the new API.
Without an OTA update, data may stop appearing or show incorrect values.

Keeping OTA enabled ensures your device stays compatible with the latest API specifications
without requiring manual firmware flashing.

> 💡 OTA updates happen automatically in the early morning hours. The device downloads and installs the update,
> then restarts. The display will show the new firmware version after the update.


