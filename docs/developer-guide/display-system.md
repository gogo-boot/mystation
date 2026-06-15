# E-Paper Display Layout Overview

<!-- TODO: Add photo of device showing Half & Half mode -->
![Half and Half display mode](/img/IMG_0872.jpeg)

## Display Hardware Specifications

- **Display Model**: 7.5 inch E-Paper Display GDEY075T7
- **Physical Resolution**: 800x480 pixels
- **Color Support**: Black & White (2-color), No Gray levels

## Layout Orientations

### Landscape Mode (800x480)

```
┌────────────────────────────────────┬─────────────────────────────────────────┐
│                                    │                                         │
│         WEATHER SECTION            │          DEPARTURE SECTION              │
│           (400x480)                │            (399x480)                    │
│                                    │                                         │
│  • City/Town Name: 22px            │  • Station Name: 17px                   │
│  • Space 20px                      │  • Space 10px                           │
│  • Day weather Info: 80px          │  • Departure Column Headers: 12px       │
│    - first column                  │  • Space 4px                            │
│       - Day Weather Icon: 48px     │  • Line 1px                             │
│        - Current Temp : 30 px      │     Left 421px for depature Entries     │
│    - second column                 │      - Depature Entries 42 px 5 times   │
│       - today low/high temp: 27px  │      - Separation Line 1px              │
│       - UV Index info: 20 px       │      - Depature Entries 42 px 5 times   │
│       - Pollen Info : 20px         │      Single Daparture Entry             │
│    - third column                  │        - Space 3px                      │
│       - Sunrise : 40 px            │        - Main Line: 17px                │
│       - Sunset : 40px              │        - Space 3px                      │
│  • Space 12px                      │        - Disruption Space: 16px         │
│  • Nächste Stunden 15px            │       - Space 3px                       │
│  • Space 25px                      │   • Footer Separation Line  1px         │
│  • Weather Graphic : 304px         │   • Footer: 15px                        │
│  • Space 12px                      │                                         │
│  • Footer: 15px                    │                                         │
└────────────────────────────────────┴─────────────────────────────────────────┘
```

## Update Performance

- **Full Screen**: Complete redraw (~2-3 seconds)
- **Partial Updates**: Not currently possible (see below)

---

## Weather Data in RTC Memory

### Why Weather Is Cached in RTC

```
Wake Cycle 1: Fetch Weather + Fetch Transport → Display → Sleep
Wake Cycle 2: RTC Weather (cached) + Fetch Transport → Display → Sleep
Wake Cycle 3: RTC Weather (cached) + Fetch Transport → Display → Sleep
Wake Cycle 4: Fetch Weather (interval expired) + Fetch Transport → Display → Sleep
```

Weather data (`WeatherInfo`) is stored in RTC memory because:

- **Weather updates are infrequent** (every 1-3 hours), while transport updates are frequent (every 5-10 minutes)
- **Avoids redundant API calls** — on most wake cycles, only transport data is fetched
- **Saves power** — one fewer HTTPS request per wake cycle reduces WiFi-on time by ~1-2 seconds
- **Required for display** — the half-and-half mode needs both weather and transport data every cycle

---

## Future Improvement: Partial Display Update

Partial display update would allow refreshing only the transport section while keeping the weather section untouched:

```
Full refresh:     [Weather ████████ | Transport ████████]  ← full flash, 2-3s
Partial refresh:  [Weather (unchanged) | Transport ████]  ← no flash on left, <1s
```

**Benefits:**

- Faster visible update (~0.5s vs 2-3s)
- No full-screen flash on every transport update
- Better user experience

**Current Blocker:**

The U8g2 font rendering library does not support partial display updates with GxEPD2.
The partial update window must avoid any U8g2-rendered text, which is used throughout both sections.
A workaround (rendering fonts to a buffer, then using GxEPD2's native partial update) has not yet been implemented.
