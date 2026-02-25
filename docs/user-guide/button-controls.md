# Button Controls

MyStation can be controlled with physical buttons. This feature allows you to temporarily
change the display mode for 2 minutes by pressing buttons, even when the device is in deep sleep.

## Button Overview

### Available Buttons

| Button                                | Function                       | Display Mode                      |
|---------------------------------------|--------------------------------|-----------------------------------|
| Button 1                              | Half & Half Temporary Mode     | Weather + Departures split screen |
| Button 1 Long press (5 seconds)       | Reset Configuration            | Application Configuration screen  |
| Button 1 and 2 Long press (5 seconds) | Factory Reset                  | Initiate factory reset            |
| Button 1 and 3 Long press (5 seconds) | Display Device Info            | Display Device Info               |
| Button 2                              | Weather Only Temporary Mode    | Full screen weather display       |
| Button 3                              | Departures Only Temporary Mode | Full screen departure display     |

**Application Configuration screen** allows you to reconfigure your weather location and departure stations
**Display Info** display firmware version, current configuration, Device Unique ID, Microcontroler type

### Temporary Mode

**Duration**: 2 minutes

- ⏰ After 2 minutes: Returns to configured mode

## Using the Buttons

### Check Both

**Press Button 1** (Half & Half Mode)

Display shows:

- Left half: Weather summary
- Right half: Departure list
- Balanced view of both information types

**Use case**: See everything at once

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

---

**Note**: Button functionality is a convenience feature. Your MyStation works perfectly fine without buttons using the
configured display mode and web interface for control.
