#pragma once
#include <Arduino.h>

// Button manager for temporary display mode switching (ESP32-S3 only)
class ButtonManager {
public:
    // Duration for temporary mode in active hours (2 minutes in seconds)
    static constexpr uint32_t TEMP_MODE_ACTIVE_DURATION = 120;

    // Debounce delay in milliseconds
    static constexpr uint32_t DEBOUNCE_DELAY_MS = 50;

    // Initialize button GPIO pins
    static void setWakupableButtons();

    // Set a synthetic button mode to be consumed by handleWakeupMode().
    // Used by long-press detection to inject a mode as if a button woke the device.
    static void setSyntheticButtonMode(int8_t mode);

    // Check which button caused EXT1 wakeup from deep sleep
    // Returns the display mode corresponding to the button, or -1 if no button
    static int8_t getWakeupButtonMode();

    // Enable EXT1 wakeup for all buttons before entering deep sleep
    static void enableButtonWakeup();

    // Handle button wakeup and set temporary display mode if needed
    // Should be called early in boot process
    static void handleWakeupMode();

    // Attach GPIO interrupts on all button pins so presses are detected while
    // the device is awake and running (not in deep sleep).
    // Call once during onInit(), after pins are configured.
    static void attachRunningInterrupts();

    // Check if a button was pressed while the device was awake (via ISR).
    // If so, inject it as a synthetic mode and trigger esp_restart() so the
    // normal handleWakeupMode() path processes it on the next boot.
    // Call in onShutdown() before entering deep sleep.
    static void checkAndHandleRunningButtonPress();

private:
    // Get GPIO mask for all three buttons
    static uint64_t getButtonMask();
};


