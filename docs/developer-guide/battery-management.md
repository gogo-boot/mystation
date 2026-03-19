# Battery Management

## Overview

MyStation supports battery voltage monitoring and percentage reporting.
The `BatteryManager` class handles ADC reading, voltage conversion, and capacity estimation.

> ⚠️ Battery monitoring is **only available** when the `SHOW_BATTERY_STATUS` build flag is enabled and the board is
> an ESP32-S3 variant with the dedicated ADC circuit.

---

## Hardware Configuration

| Parameter             | Value                | Description                             |
|-----------------------|----------------------|-----------------------------------------|
| ADC Pin               | GPIO1 (A0) — BAT_ADC | Battery voltage sense input             |
| ADC Enable Pin        | GPIO6 (A5) — ADC_EN  | Controls power to the ADC circuit       |
| ADC Resolution        | 12-bit (0 – 4095)    | —                                       |
| ADC Reference Voltage | 3.6 V                | Actual reference for ESP32-S3           |
| Voltage Divider Ratio | 2 : 1                | Onboard resistor divider halves voltage |
| Calibration Factor    | 0.968                | From OG DIY Kit reference               |

**Voltage formula:**

```
V_bat = (ADC_raw / 4095) × 3.6 V × 2.0 × 0.968
```

Readings are averaged over **10 samples** to reduce ADC noise. The ADC enable pin is driven HIGH only during a
reading and immediately returned LOW to save power.

---

## Voltage Thresholds

| Threshold                 | Voltage   | Capacity | Notes                                               |
|---------------------------|-----------|----------|-----------------------------------------------------|
| Fully charged             | 4.120 V   | 100 %    | `BATTERY_VOLTAGE_MAX` constant                      |
| Nominal                   | 3.643 V   | ~50 %    | `BATTERY_VOLTAGE_NOMINAL` constant                  |
| ⚠️ Low battery warning    | ≈ 3.252 V | **5 %**  | Trigger warning UI; prompt user to recharge         |
| ⚡ ESP32 instability zone  | < 3.0 V   | < 0 %    | WiFi & Bluetooth become unreliable below this point |
| 🔒 Hardware power cut-off | 3.0 V     | 0 %      | `BATTERY_VOLTAGE_MIN`; onboard circuit cuts power   |

### Why 3.252 V as the Warning Threshold?

The discharge curve (see below) maps **5 % capacity to approximately 3.252 V**.
At this level the battery still has enough energy for one more update cycle, giving the user actionable time to
recharge before the hardware cut-off at 3.0 V.

### ESP32 Stability Below 3.0 V

Although an ESP32 may remain partially functional below 3.0 V, RF peripherals (WiFi, Bluetooth) become unstable.
Running the chip at such low voltages is **not recommended** and may cause:

- Failed WiFi associations or dropped connections
- Corrupted NVS write operations
- Unpredictable deep-sleep wake behaviour
- Accelerated battery wear

---

## Li-ion Discharge Curve

The values below are measured from a typical Li-ion cell and are used internally for capacity estimation.

| Capacity (%) | Voltage (V) | Notes                              |
|--------------|-------------|------------------------------------|
| 100          | 4.111       | Discharge curve top (measured)     |
| 90           | 4.020       |                                    |
| 80           | 3.950       |                                    |
| 70           | 3.840       |                                    |
| 60           | 3.750       |                                    |
| **50**       | **3.643**   | Nominal voltage                    |
| 40           | 3.580       |                                    |
| 30           | 3.500       |                                    |
| 20           | 3.420       |                                    |
| 10           | 3.350       |                                    |
| **5**        | **3.252**   | **⚠️ Low battery warning trigger** |
| 2            | 3.120       |                                    |
| **1**        | **2.795**   | Discharge curve bottom (measured)  |

> 📝 **Current implementation** uses a linear interpolation between `BATTERY_VOLTAGE_MIN` (3.010 V) and
> `BATTERY_VOLTAGE_MAX` (4.120 V). The table above documents the *real* non-linear Li-ion discharge curve and can be
> used to implement a look-up-table (LUT) approach in a future improvement.

---

## Battery Percentage Calculation

### Current Implementation (Linear)

```cpp
// voltageToPercentage() in battery_manager.cpp
float percentage = ((voltage - BATTERY_VOLTAGE_MIN) / (BATTERY_VOLTAGE_MAX - BATTERY_VOLTAGE_MIN)) * 100.0f;
```

- Simple and fast, but underestimates capacity in the flat mid-range (~3.6 – 3.9 V)
- Overestimates capacity near the knee of the curve (< 3.3 V)

### Recommended Future Improvement (LUT)

A look-up table based on the measured discharge curve (1 % → 2.795 V, 100 % → 4.111 V) would provide significantly
more accurate readings:

```cpp
// Example LUT approach (not yet implemented)
static const float VOLTAGE_LUT[] = {
    2.795f, // 1%
    3.120f, // 2%
    3.252f, // 5%  ← low-battery warning threshold
    3.350f, // 10%
    3.420f, // 20%
    3.500f, // 30%
    3.580f, // 40%
    3.643f, // 50% ← nominal
    3.750f, // 60%
    3.840f, // 70%
    3.950f, // 80%
    4.020f, // 90%
    4.111f, // 100%
};
```

---

## Battery Icon Levels

The icon level maps the percentage to one of five visual states:

| Icon Level | Percentage Range | Meaning       |
|------------|------------------|---------------|
| 5          | 80 – 100 %       | Full          |
| 4          | 60 – 79 %        | High          |
| 3          | 40 – 59 %        | Medium        |
| 2          | 20 – 39 %        | Low           |
| 1          | 0 – 19 %         | Critical      |
| 0          | N/A              | Not available |

---

## Charging Detection

Charging is inferred by comparing voltage against `BATTERY_VOLTAGE_MAX + 0.1 V` (4.22 V).
This is a heuristic — no dedicated charge-status pin is read.

```cpp
return voltage > (BATTERY_VOLTAGE_MAX + 0.1f);  // > 4.22 V → likely charging
```

---

## Usage Example

```cpp
BatteryManager::init(); // Call once in setup()

if (BatteryManager::isAvailable()) {
    float  voltage    = BatteryManager::getBatteryVoltage();    // e.g. 3.72 V
    int    percentage = BatteryManager::getBatteryPercentage(); // e.g. 63
    int    iconLevel  = BatteryManager::getBatteryIconLevel();  // e.g. 4
    bool   charging   = BatteryManager::isCharging();

    if (percentage <= 5) {
        // Trigger low battery warning on display
    }
}
```

---

## Temperature Management

Battery temperature is managed **autonomously by the SY6974B PMIC** via the **NTC (Negative Temperature Coefficient) pin
** — no firmware intervention is required.

The SY6974B features integrated hardware comparators that continuously monitor the NTC thermistor resistance. If the
battery temperature exceeds safe thresholds, the PMIC will **automatically suspend charging** without any software
involvement.

| Event                             | Behaviour                                       |
|-----------------------------------|-------------------------------------------------|
| Temperature within safe range     | Charging proceeds normally                      |
| Temperature too high / too low    | Charging is automatically suspended by hardware |
| Temperature returns to safe range | Charging resumes automatically                  |

> ℹ️ Because this is handled entirely in hardware, there is no temperature API in `BatteryManager` and no firmware
> action needed. The firmware does not need to read or react to battery temperature.

---

## Related Files

| File                             | Description                            |
|----------------------------------|----------------------------------------|
| `include/util/battery_manager.h` | Class declaration and constants        |
| `src/util/battery_manager.cpp`   | ADC reading, voltage conversion, icons |
| `include/config/pins.h`          | `BATTERY_ADC` and `ADC_EN` pin mapping |
| `include/build_config.h`         | `SHOW_BATTERY_STATUS` compile flag     |

---

## References

- [TRMNL OG DIY Kit — Arduino Wiki](https://wiki.seeedstudio.com/ogdiy_kit_works_with_arduino/)
- [ESP32-S3 ADC Characteristics — Espressif Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/adc_calibration.html)


