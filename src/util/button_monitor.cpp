#include "util/button_monitor.h"
#include "build_config.h"
#include "config/pins.h"

uint8_t ButtonMonitor::detectLongPress(unsigned long durationMs) {
    if (!HAS_BUTTON) {
        return BUTTON_NONE;
    }

    // Initialize button pins before reading
    pinMode(Pins::GPIO_BUTTON_1, INPUT_PULLUP);
    pinMode(Pins::GPIO_BUTTON_2, INPUT_PULLUP);
    pinMode(Pins::GPIO_BUTTON_3, INPUT_PULLUP);
    delay(20); // Allow pull-ups to stabilize

    // Sample initial state — only track buttons that are already pressed (LOW)
    bool b1Active = (digitalRead(Pins::GPIO_BUTTON_1) == LOW);
    bool b2Active = (digitalRead(Pins::GPIO_BUTTON_2) == LOW);
    bool b3Active = (digitalRead(Pins::GPIO_BUTTON_3) == LOW);

    // If nothing is pressed at all, return immediately
    if (!b1Active && !b2Active && !b3Active) {
        return BUTTON_NONE;
    }

    Serial.println("🔵 Button(s) detected — monitoring for long press...");

    unsigned long startTime = millis();
    unsigned long lastProgressTime = 0;

    while (millis() - startTime < durationMs) {
        // If a button is released, it is no longer eligible
        if (b1Active && digitalRead(Pins::GPIO_BUTTON_1) == HIGH) {
            b1Active = false;
        }
        if (b2Active && digitalRead(Pins::GPIO_BUTTON_2) == HIGH) {
            b2Active = false;
        }
        if (b3Active && digitalRead(Pins::GPIO_BUTTON_3) == HIGH) {
            b3Active = false;
        }

        // All buttons released early — nothing to trigger
        if (!b1Active && !b2Active && !b3Active) {
            unsigned long held = millis() - startTime;
            Serial.printf("🟢 All buttons released after %.1f s (not long enough)\n",
                          held / 1000.0f);
            return BUTTON_NONE;
        }

        // Progress feedback every second
        unsigned long elapsed = millis() - startTime;
        if (elapsed - lastProgressTime >= 1000) {
            unsigned long remaining = (durationMs - elapsed) / 1000;
            if (remaining > 0) {
                Serial.printf("⏱️  Holding... %lu second(s) remaining\n", remaining);
            }
            lastProgressTime = elapsed;
        }

        delay(50); // debounce polling interval
    }

    // Build result bitmask from buttons that survived the full duration
    uint8_t result = BUTTON_NONE;
    if (b1Active) result |= BUTTON_1_HELD;
    if (b2Active) result |= BUTTON_2_HELD;
    if (b3Active) result |= BUTTON_3_HELD;

    Serial.printf("✅ Long press detected — bitmask: 0x%02X\n", result);
    return result;
}

