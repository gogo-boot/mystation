# Button Controls

MyStation can be controlled with physical buttons. This feature allows you to temporarily
change the display mode for 2 minutes by pressing buttons, even when the device is in deep sleep.

## Button Overview

### Available Buttons

| Button   | Function             | Display Mode                      |
|----------|----------------------|-----------------------------------|
| Button 1 | Half & Half Mode     | Weather + Departures split screen |
| Button 2 | Weather Only Mode    | Full screen weather display       |
| Button 3 | Departures Only Mode | Full screen departure display     |

### Additional Function

**Button 1** also serves as the Factory Reset button:

- **Short press** (< 1 second): Switch to Half & Half display mode
- **Long press** (5 seconds during boot): Initiate factory reset

## How Buttons Work

### Button Press Behavior

When you press a button while device is sleeping:

1. **Mystation wakes up** immediately from deep sleep
1. **Detects which button** triggered the wake-up
1. **Enters temporary mode** with selected display style
1. **Connects to WiFi** (2.4 GHz network)
1. **Fetches fresh data** (weather and/or departures)
1. **Updates display** showing requested information
1. Deep sleeps for 2 minutes
1. **Wake up and Returns to normal** operation

### Temporary Mode

**Duration**: 2 minutes (120 seconds)

During temporary mode:

- ✅ Buttons remain active (you can press again)
- ✅ Display shows selected mode
- ✅ Can switch between modes freely
- ✅ Fresh data is fetched
- ⏰ After 2 minutes: Returns to configured mode

**Why temporary?**

- Saves battery life
- Prevents accidentally staying in wrong mode
- Normal schedule resumes automatically

## Using the Buttons

### Check Current Weather

**Press Button 2** (Weather Only Mode)

Display shows:

- Current temperature and "feels like"
- Weather icon and description
- 24-hour temperature graph
- Wind, humidity, precipitation
- Sunrise/sunset times

**Use case**: Quick weather check before leaving house

### Check Departures

**Press Button 3** (Departures Only Mode)

Display shows:

- List of upcoming departures
- Departure times with delays
- Platform/track information
- Transport type (RE, S-Bahn, Bus, etc.)
- Destination station

**Use case**: Check when next train/bus arrives

### Check Both

**Press Button 1** (Half & Half Mode)

Display shows:

- Top half: Weather summary
- Bottom half: Departure list
- Balanced view of both information types

**Use case**: See everything at once

### Factory Reset

**Press and hold Button 1 during power-on** for 5 seconds

## Display Modes Explained

### Mode 1: Half & Half (Button 1)

**Layout**:

```
┌─────────────────────────────┐
│ Weather Info │ Departures   │
│ Temp, Icon,  │ Next trains  │
│ Forecast     │ buses, etc.  │
└──────────────┴──────────────┘
```

**Best for**: Balanced information, daily use

### Mode 2: Weather Only (Button 2)

**Layout**:

```
┌─────────────────────────────┐
│                             │
│   Large Weather Display     │
│   Temperature Graph         │
│   Detailed Forecast         │
│   Wind, Humidity, etc.      │
│                             │
└─────────────────────────────┘
```

**Best for**: Weather enthusiasts, outdoor planning

### Mode 3: Departures Only (Button 3)

**Layout**:

```
┌─────────────────────────────┐
│                             │
│   Departure List            │
│   More departures visible   │
│   Detailed timing info      │
│   Platform numbers          │
│                             │
└─────────────────────────────┘
```

**Best for**: Commuters, catching transport

## Related Documentation

- 🏠 [User Guide Home](index.md)
- 🔧 [Hardware Assembly](index.md) - Button wiring instructions
- 📋 [Troubleshooting](troubleshooting.md) - Button issues and solutions

---

**Note**: Button functionality is a convenience feature. Your MyStation works perfectly fine without buttons using the
configured display mode and web interface for control.
