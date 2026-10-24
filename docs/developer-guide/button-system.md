# Button System

## Overview

MyStation uses three physical buttons (GPIO pins with internal pull-ups) for user interaction.
Buttons serve two purposes:

1. **Short press** — temporarily switch the display mode (2 minutes)
2. **Long press (3 seconds)** — trigger system actions (config mode, device info, OTA update)

## Hardware Wiring

Buttons are wired between GPIO and GND (normally open, momentary push buttons):

```
3.3V ──[internal pull-up]── GPIO Pin ──[Button]── GND
```

- **Not pressed**: pin reads HIGH (pulled up to 3.3V)
- **Pressed**: pin reads LOW (shorted to GND through button)

## FALLING Edge Interrupts

The system uses **FALLING edge interrupts** to detect button presses during normal operation:

```
Voltage
  HIGH ──────┐                    ┌──────
             │  FALLING edge      │
  LOW        └────────────────────┘
             ↑ ISR fires here     ↑ RISING edge (ignored)
         (button pressed)     (button released)
```

`FALLING` = the signal transitions from HIGH → LOW = the moment the user presses the button down.

The ISR (Interrupt Service Routine) is a tiny function that runs immediately when the hardware
detects this edge. It simply records which button was pressed into a volatile variable:

```cpp
static void IRAM_ATTR isrButton2() {
    if (isrButtonPressed < 0) isrButtonPressed = DISPLAY_MODE_WEATHER_ONLY;
}
```

The main code later checks `isrButtonPressed` and acts on it.

## Mechanical Bounce

Physical buttons do not make clean electrical contact. When pressed or released, the metal
contacts vibrate for 1–10 milliseconds, rapidly toggling between states:

```
Press:
  HIGH ────┐ ┌┐ ┌┐
           └─┘└─┘└────── LOW (stable, held)
           ↑ bounce

Release:
  LOW ─────────────┐┌─┐┌─┐┌──── HIGH (stable, released)
                   └┘ └┘ └┘
                   ↑ bounce (creates FALLING edges!)
```

**The release bounce is the critical problem**: each LOW→HIGH→LOW bounce creates a FALLING edge
that triggers the ISR, registering a phantom "button press."

### Debouncing Strategies

1. **Time-based debounce** (used for ISR): ignore triggers within 500ms of attaching interrupts
2. **Wait-for-release** (used after long press): poll until pin is stable HIGH for 50ms

## Short Press Flow (Normal Operation)

During normal operation, FALLING edge interrupts detect button presses:

```
Deep Sleep → [Button pressed] → EXT1 Wakeup → Boot → Normal cycle
                                                     → Display temp mode (2 min)
```

Or during an active wake cycle:

```
ISR fires → checkAndRestartIfButtonPressed() → Save to NVS → esp_restart()
→ On reboot: load pending temp mode from NVS → Display temp mode (2 min)
```

## Long Press Flow (System Actions)

Long press detection happens early in boot, before interrupts are attached:

```
Deep Sleep → [Button pressed] → EXT1 Wakeup → Boot
  → handleButtonLongPressActions()
    → detectLongPress(3000ms) — polls button for 3 seconds
    → If still held: execute long-press action
    → waitForButtonRelease() — wait for clean release ← FIX
  → attachRunningInterrupts() — now safe to listen for new presses
```

### Why Wait for Release?

Without waiting, this happens:

1. Long press detected (Button 2 held 3s) → Application Info mode set
2. Interrupts attached (FALLING edge)
3. User releases button → bounce creates FALLING edge → ISR fires
4. System thinks a new short press occurred → switches to Weather Full
5. User sees Application Info flash briefly, then Weather Full appears

The `waitForButtonRelease()` function blocks until the pin reads HIGH for 50ms consecutive,
ensuring all bounce has settled before the ISR is armed.

## Synthetic Mode

Normally, the system determines which button woke it from deep sleep by reading the
**EXT1 wakeup pin mask** — hardware that records which GPIO was LOW at wakeup time.

But this only tells us *which* button — not *how long* it was held. Long press detection
happens after wakeup, in software. Once a long press action is determined, the code needs
to route to a different display mode than what the EXT1 mask would indicate.

**Synthetic mode** solves this by manually injecting a "fake" button press:

```cpp
// Instead of reading hardware wakeup mask:
ButtonManager::setSyntheticButtonMode(DISPLAY_MODE_APPLICATION_INFO);
```

Later in the boot sequence, `handleWakeupMode()` checks synthetic mode first:

```cpp
int8_t buttonMode = syntheticButtonMode;  // Check synthetic first
if (buttonMode >= 0) {
    syntheticButtonMode = -1;  // Consume it
} else {
    buttonMode = getWakeupButtonMode();  // Fall back to hardware
}
```

This cleanly separates "what woke the device" from "what action to take."

## Button Actions Summary

### Default Behavior (Half & Half / Transport Only)

| Button | Short Press | Long Press (3s) |
|--------|-------------|-----------------|
| Button 1 | Half & Half mode (temp) | Enter Configure Mode (restarts) |
| Button 2 | Weather Full mode (temp) | Show Application Info (temp) |
| Button 3 | Transport Full mode (temp) | Trigger OTA Update |
| Button 1+2 | — | Factory Reset (restarts) |

> **Weather browse from Half & Half.** Day browse is also reachable from configured
> Half & Half, whenever the **Weather-Full view is effectively showing**:
>
> - During transport-active hours, Half & Half shows the split view. The first **Button 2**
>   switches to the temporary **Weather Full** view; a **second Button 2** (now that
>   Weather Full is showing) **enters weather browse** (day 1).
> - Outside transport-active hours, Half & Half already renders weather-full (the
>   time-based fallback), so the **first Button 2 enters weather browse** directly.
>
> While browsing, **Button 2 = +1 day** and **Button 3 = −1 day** (circular), exactly like
> Weather-Only browse. **Button 1 exits browse and shows the Half & Half split view**
> (forced via a temporary mode so the screen visibly changes even outside transport hours;
> it reverts to the time-based default on the next wake). **Solar browse is not offered**
> from this path — it remains exclusive to configured Weather-Only mode.

### Weather-Only Mode (Day Browse)

In Weather-Only mode the short-press behavior depends on **whether the device is already
browsing** (temporary browse mode active) and, if so, on the active **browse context**
(weather browse or solar browse). Long-press actions are unchanged in every state.

**From the default weather-today view (not browsing):**

| Button | Short Press | Long Press (3s) |
|--------|-------------|-----------------|
| Button 1 | Stay on weather today (day 0) | Enter Configure Mode (restarts) |
| Button 2 | Enter **weather browse** (starts at day 1) | Show Application Info (temp) |
| Button 3 | Enter **solar browse** (starts at day 0 = today) | Trigger OTA Update |
| Button 1+2 | — | Factory Reset (restarts) |

**While already browsing (either context):**

| Button | Short Press | Long Press (3s) |
|--------|-------------|-----------------|
| Button 1 | Exit browsing → default weather-today view | Enter Configure Mode (restarts) |
| Button 2 | Next day (+1, circular, same context) | Show Application Info (temp) |
| Button 3 | Previous day (-1, circular, same context) | Trigger OTA Update |
| Button 1+2 | — | Factory Reset (restarts) |

> Long press actions are unchanged — only short press behavior is reinterpreted in
> Weather-Only mode. Switching context (weather ↔ solar) is only possible from the
> default view: press Button 1 to return to default, then Button 2 or Button 3 to enter
> the other context.

---

## Weather-Only Day Browse

When the effective display mode is **Weather-Only** (`getEffectiveDisplayMode() ==
DISPLAY_MODE_WEATHER_ONLY`), the three physical buttons are reinterpreted to **browse** the
forecast instead of switching display modes. Long press actions (configure mode, application
info, OTA) remain unchanged.

### Activation Condition

Day browse activates when **all** of these are true:

- Effective display mode is `DISPLAY_MODE_WEATHER_ONLY` (via
  `TimingManager::getEffectiveDisplayMode()`) — not merely the *configured* mode
- The resolved button mode is **not** `DISPLAY_MODE_APPLICATION_INFO` (long-press guard)

Because the guard uses the **effective** mode, day browse works when Weather-Only is the
configured mode, when Half & Half falls back to weather-full outside transport hours, **and
when a temporary Weather Full is active** (entered from Half & Half by a first Button 2 — a
second Button 2 then enters browse).

### Two Browse Contexts

Day browse now has **two contexts**, tracked by the `RTC_DATA_ATTR` field
`config.browseContext` (`enum BrowseContext { BROWSE_WEATHER = 0, BROWSE_SOLAR = 1 }`):

| Context | Umbrella term | What it shows | Day range |
|---------|---------------|---------------|-----------|
| `BROWSE_WEATHER` | **weather browse** | Per-day temperature + rain graph | Days 1..max-1 (skips today) |
| `BROWSE_SOLAR` | **solar browse** | Per-day solar radiation graph ("Sonnenstrom") | Days 0..max-1 (includes today) |

Both are variants of the same **day browse** feature; only the rendered graph and the
day range differ.

### Enter vs. Step: the `inWeatherBrowse` Signal

The meaning of a press depends on whether the device is **already browsing**, tracked by the
explicit `RTC_DATA_ATTR` flag **`config.inWeatherBrowse`** (an earlier design inferred this
from `inTemporaryMode`/`selectedForecastDay`, which was error-prone when Half & Half fell back
to weather-full — the explicit flag removes that ambiguity).

Both button paths funnel through one shared state machine,
`ButtonManager::applyWeatherBrowsePress(config, buttonMode)`, which returns a `BrowseResult`:

| Result | Meaning | Caller action |
|--------|---------|---------------|
| `BROWSE_NONE` | not a browse action | normal temporary mode switch |
| `BROWSE_ENTER_OR_STEP` | entered browse or stepped a day | keep a temporary Weather Full |
| `BROWSE_EXIT` | left browse | show the configured layout |

**Not browsing (`inWeatherBrowse == false`, Weather Full showing):**

```
Button 2 (Weather pin)     → ENTER weather browse (context = WEATHER, day = 1)  [ENTER_OR_STEP]
Button 1 / Button 3        → not a browse action                                [NONE]
                             (configured Weather-Only: B3 enters solar browse instead;
                              see the Weather-Only tables above)
```

**Already browsing (`inWeatherBrowse == true`):**

```
Button 2 (Weather pin)     → next day (+1), circular                            [ENTER_OR_STEP]
Button 3 (Transport pin)   → previous day (-1), circular                        [ENTER_OR_STEP]
Button 1 (Half & Half pin) → EXIT browse                                        [EXIT]
                             - configured Half & Half: FORCE the split view
                             - configured Weather-Only: weather-today (day 0)
```

Solar browse (`BROWSE_SOLAR`) is only entered from the configured **Weather-Only** default
view (Button 3); it is never offered when browsing is reached from Half & Half.

### Wrapping Logic (context-dependent)

Day navigation is **circular** and the lower bound depends on the context — solar browse
includes today (day 0), weather browse skips it:

```
Weather browse (skips day 0):
  Forward  (B2):  1 → 2 → 3 → 4 → 5 → 6 → 1   (wraps to 1)
  Backward (B3):  1 → 6 → 5 → 4 → 3 → 2 → 1   (wraps to max-1)

Solar browse (includes day 0):
  Forward  (B2):  0 → 1 → 2 → 3 → 4 → 5 → 6 → 0  (wraps to 0)
  Backward (B3):  0 → 6 → 5 → 4 → 3 → 2 → 1 → 0  (wraps to max-1)
```

The step is computed by the `stepBrowseDay()` helper in `button_manager.cpp`: the lower
bound is `0` for `BROWSE_SOLAR` and `1` for `BROWSE_WEATHER`; the upper bound is
`availableForecastDays - 1` (set from `weather.dailyForecastCount` after fetch).
Models with fewer forecast days have a smaller range:

| Weather Model | Available Days | Weather browse range | Solar browse range |
|---------------|----------------|----------------------|--------------------|
| Auto / DWD ICON / ECMWF | 7 | Day 1-6 | Day 0-6 |
| MeteoSwiss | 5 | Day 1-4 | Day 0-4 |
| Meteo-France | 4 | Day 1-3 | Day 0-3 |
| ItaliaMeteo | 3 | Day 1-2 | Day 0-2 |

Button 1 resets `selectedForecastDay` to 0 and `browseContext` to `BROWSE_WEATHER`, then
shows the configured layout: **configured Weather-Only** returns to weather-today; **configured
Half & Half** shows the split view (forced via a temporary mode).

### Implementation: Two Button Paths

Both paths call the **same** state machine, `applyWeatherBrowsePress()`, then apply
temp-mode bookkeeping based on its `BrowseResult`. Keeping the decision in one place is why
the wakeup path and the awake path can't drift apart.

#### Deep Sleep Wakeup Path

When a button wakes the device from deep sleep, `handleWakeupMode()` runs the state machine
and acts on the result:

```cpp
BrowseResult br = applyWeatherBrowsePress(config, buttonMode); // guards on effective mode

if (br == BROWSE_ENTER_OR_STEP) {
    // keep a temporary Weather Full so the browse view stays on screen
    config.inTemporaryMode = true;
    config.temporaryDisplayMode = DISPLAY_MODE_WEATHER_ONLY;
} else if (br == BROWSE_EXIT) {
    if (config.displayMode == DISPLAY_MODE_HALF_AND_HALF) {
        // force the split view (visible feedback even outside transport hours)
        config.inTemporaryMode = true;
        config.temporaryDisplayMode = DISPLAY_MODE_HALF_AND_HALF;
    } else {
        config.inTemporaryMode = false;   // Weather-Only: back to weather-today
    }
} else { // BROWSE_NONE
    config.inTemporaryMode = true;        // normal temporary mode switch
    config.temporaryDisplayMode = (uint8_t)buttonMode;
}
```

#### Awake Press Path (ISR)

When a button is pressed during an active wake cycle, the ISR fires and
`checkAndRestartIfButtonPressed()` runs the **same** state machine, then `esp_restart()`s.
Because the restart path repopulates RTC from NVS on the next boot, the resulting browse
state is saved to transient NVS keys before restarting:

```
ISR fires → checkAndRestartIfButtonPressed()
  → applyWeatherBrowsePress(config, mode)   (same helper as the wakeup path)
  → Save "pendingTempMode" / "pendingDay" / "pendingCtx" / "pendingBrowse" to NVS
    (BROWSE_EXIT on configured Weather-Only skips persisting → reverts to configured mode)
  → esp_restart()
  → On reboot: handleWakeupMode() loads the pending keys, deletes them, applies to RTC
```

The `pendingTempMode`, `pendingDay`, `pendingCtx`, and `pendingBrowse` NVS keys are transient
— read once and immediately deleted after loading. This avoids polluting NVS with persistent
state for a runtime-only feature.

### Temporary Mode Expiry

When temporary mode expires (2 minutes after button press), `selectedForecastDay` is
automatically reset to 0. This ensures the device returns to showing today's weather
after the browsing session ends, rather than staying on a stale future day.

```
Button press → selectedForecastDay = 3 → display day 3
  ... 2 minutes pass ...
Temp mode expires → selectedForecastDay reset to 0 → display today
```

## Key Code Files

| File | Purpose |
|------|---------|
| `src/util/button_monitor.cpp` | Long press detection (polling loop) |
| `src/util/button_manager.cpp` | ISR handlers, wakeup mode, temp mode, day browse reinterpretation, `applyWeatherBrowsePress()` state machine, `stepBrowseDay()`, browse context |
| `src/util/system_init.cpp` | Long press action routing, wait-for-release |
| `include/util/button_manager.h` | `BrowseResult` enum, `applyWeatherBrowsePress()` declaration |
| `include/config/pins.h` | GPIO pin assignments per board |
| `include/config/config_manager.h` | `BrowseContext` enum, `browseContext` / `selectedForecastDay` / `inWeatherBrowse` RTC fields |
| `src/display/weather_general_full.cpp` | Day browse layouts (`drawDayBrowseLayout()` weather, `drawSolarBrowseLayout()` solar) |
| `src/display/weather_graph.cpp` | Solar radiation curve (`drawSolarRadiationGraph()`) |
| `src/display/solar_math.cpp` | `calculateSolarAxisMax()`, `findSolarPeak()`, `hasValidSolarData()` |
| `src/api/dwd_weather_api.cpp` | Multi-day hourly fetch for the RTC cache (`getWeatherHourlyMultiDay()`), solar radiation fetch |
