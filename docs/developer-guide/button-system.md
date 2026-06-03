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

| Button | Short Press | Long Press (3s) |
|--------|-------------|-----------------|
| Button 1 | Half & Half mode (temp) | Enter Configure Mode (restarts) |
| Button 2 | Weather Full mode (temp) | Show Application Info (temp) |
| Button 3 | Transport Full mode (temp) | Trigger OTA Update |
| Button 1+2 | — | Factory Reset (restarts) |

## Key Code Files

| File | Purpose |
|------|---------|
| `src/util/button_monitor.cpp` | Long press detection (polling loop) |
| `src/util/button_manager.cpp` | ISR handlers, wakeup mode, temp mode |
| `src/util/system_init.cpp` | Long press action routing, wait-for-release |
| `include/config/pins.h` | GPIO pin assignments per board |
