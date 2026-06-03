# Configuration Guide

This page explains every setting in the MyStation settings screen.

To open the settings screen, hold **Button 1 for 5 seconds** to enter Configure Mode.
Not sure how? See [Configure Mode](configure-mode.md).

---

## Display Mode

Choose what appears on the screen during normal use.

| Mode               | What you see                                                   |
|--------------------|----------------------------------------------------------------|
| **Half & Half**    | Weather on one side, transport departures on the other         |
| **Weather Full**   | Full screen weather details with a 12-hour forecast graph      |
| **Transport Full** | Full screen list of upcoming departures — more entries visible |

> 💡 This is your **permanent** setting. Pressing the buttons switches the view temporarily
> for 2 minutes, then it returns to this setting. See [Button Controls](button-controls.md).

---

## Location

### How MyStation Finds Your Location

When you open the settings page, MyStation automatically estimates your location
using nearby WiFi networks. It uses this to:

- Find public transport stops close to you
- Set up the weather for your area

This estimated location is shown on the settings page. In most cases it is accurate
enough and you don't need to change anything.

### City Name vs. Auto-Detected Location

You can also type in a city or town name manually. However, **auto-detection is more accurate**:

> 💡 **Why auto-detect is better:**
> When you type a city name, the weather is calculated for the **exact centre of that city**.
> If you live in a suburb or on the edge of town, that centre point may be several kilometres away,
> which can lead to slightly different rain or temperature readings.
>
> The auto-detected location is based on where your WiFi networks are, which places the point
> much closer to your actual home — giving you more accurate local weather.

**In practice:**

- ✅ Leave the auto-detected location if it looks roughly correct
- ✏️ Type your city name only if the auto-detection shows a completely wrong area

---

## Weather Settings

### How Often to Update the Weather

This controls how frequently MyStation checks for new weather data.

| Interval                | Battery impact | Good for              |
|-------------------------|----------------|-----------------------|
| 1 hour                  | Higher         | Very frequent updates |
| 2 hours                 | Medium         |                       |
| **3 hours** *(default)* | **Balanced**   | **Most users**        |
| 6 hours                 | Low            | Battery saving        |
| 12 hours                | Lowest         | Maximum battery life  |

> 🔋 Weather changes slowly. Checking every 3 hours is enough for most daily decisions.

### Weather Model

You can choose which weather model MyStation uses. Different models cover different regions
and provide different forecast lengths.

| Model                        | Forecast | Best for              |
|------------------------------|----------|-----------------------|
| **Automatisch** *(default)*  | 7 days   | Works everywhere      |
| DWD ICON (Deutschland)       | 7 days   | Living in Germany     |
| ECMWF (Europa)               | 7 days   | General Europe        |
| Meteo-France (Frankreich)    | 4 days   | Living in France      |
| MeteoSwiss (Schweiz)         | 5 days   | Living in Switzerland |
| ItaliaMeteo (Italien)        | 3 days   | Living in Italy       |

> 💡 If you choose a model with fewer than 7 days forecast, the missing days will simply
> be left empty on the display. The "Automatisch" option works well for most users.

---

## Transport Settings

### Selecting Your Stop

The settings page shows a list of nearby transport stops, sorted by how close they are to you.
Simply select the stop you use most often.

If you don't see your stop, check that your location was detected correctly.

### How Often to Update Departures

This controls how frequently MyStation checks for new departure times.

| Interval                  | Battery impact | Good for             |
|---------------------------|----------------|----------------------|
| 1 minute                  | Much higher    | Near real-time       |
| 3 minutes                 | Higher         | Frequent             |
| **5 minutes** *(default)* | **Balanced**   | **Most users**       |
| 10 minutes                | Lower          | Battery saving       |
| 15 minutes                | Low            |                      |
| 30 minutes                | Lowest         | Maximum battery life |

> 🔋 Departure times can change with delays, so shorter intervals are more useful here
> than for weather. 5 minutes is a good balance.

### Active Hours for Transport Updates

You can limit transport updates to certain hours of the day — for example, only during
your morning commute. Outside those hours, the device skips transport updates to save battery.

Example: Active from `06:00` to `09:00`

### Walking Time to the Stop

Enter how many minutes it takes you to walk to your stop.
MyStation uses this to omits which departures are not catchable.

### Transport Types

Choose which types of transport to show. Only enable the types that actually serve your stop:

| Type   | What it is                         |
|--------|------------------------------------|
| RE     | Regional Express / Regional trains |
| S-Bahn | Suburban railway                   |
| U-Bahn | Underground / Metro                |
| Tram   | Straßenbahn                        |
| Bus    | Regular bus                        |

---

## Sleep Schedule

Set hours during which the device goes into deep sleep and stops updating entirely.
This is the **most effective way to extend battery life**.

Example: Sleep from `22:30` to `05:30`

> 💡 The e-paper screen keeps showing the last content even while the device is sleeping —
> it doesn't need power to hold the image.

---

## Weekend Mode

Set different active hours and sleep times for weekends (Saturday and Sunday).
Useful if you have a different routine on weekends.

Example: Stay awake later on weekends — sleep from `23:00` to `07:00`.

---

## Automatic Software Updates

MyStation can update its own internal software automatically while you sleep.

| Setting         | Default                                                |
|-----------------|--------------------------------------------------------|
| Updates enabled | ✅ Yes                                                  |
| Update time     | Randomly set between 1:00 am and 4:59 am on first boot |

### Why Updates Matter

The services that provide weather data and transport times (German Weather Service and RMV)
occasionally change how they send data. When this happens, MyStation needs a software update
to keep working correctly.

Without updates, the weather or departure information may eventually stop appearing or show
incorrect values.

> 💡 Updates happen quietly in the early morning hours. The device downloads the update,
> installs it, and restarts on its own. You'll see the updated software version the next time
> you check device info (hold Button 2 for 5 seconds).

We recommend keeping automatic updates **turned on**.
