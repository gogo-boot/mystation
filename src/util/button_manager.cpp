#include "util/button_manager.h"
#include "build_config.h"
#include "config/pins.h"
#include "config/config_manager.h"
#include <driver/rtc_io.h>
#include <esp_system.h>
#include <Preferences.h>

static const char* TAG = "BUTTON";

// Synthetic button mode injected by long-press detection (-1 = none)
static int8_t syntheticButtonMode = -1;

// Volatile flag set by ISR when a button is pressed while device is awake.
// -1 = no press, 0/1/2 = display mode.
static volatile int8_t isrButtonPressed = -1;

// Timestamp when interrupts were attached — used for debounce
static unsigned long isrAttachTime = 0;

void ButtonManager::setSyntheticButtonMode(int8_t mode) {
    syntheticButtonMode = mode;
    ESP_LOGI(TAG, "Synthetic button mode set: %d", mode);
}

void ButtonManager::setWakupableButtons() {
    if (HAS_BUTTON) {
        ESP_LOGI(TAG, "Initializing button manager...");

        // Configure button pins as input with internal pull-up
        pinMode(Pins::BUTTON_HALF_AND_HALF, INPUT_PULLUP);
        pinMode(Pins::BUTTON_WEATHER_ONLY, INPUT_PULLUP);
        pinMode(Pins::BUTTON_DEPARTURE_ONLY, INPUT_PULLUP);

        ESP_LOGI(TAG, "Button pins configured: GPIO %d, %d, %d",
                 Pins::BUTTON_HALF_AND_HALF,
                 Pins::BUTTON_WEATHER_ONLY, Pins::BUTTON_DEPARTURE_ONLY);

        // DIAGNOSTIC: Check if GPIOs support RTC (required for EXT1 wakeup)
        if (rtc_gpio_is_valid_gpio((gpio_num_t)Pins::BUTTON_HALF_AND_HALF)) {
            ESP_LOGI(TAG, "✓ GPIO %d supports RTC", Pins::BUTTON_HALF_AND_HALF);
        } else {
            ESP_LOGE(TAG, "✗ GPIO %d does NOT support RTC!", Pins::BUTTON_HALF_AND_HALF);
        }
        if (rtc_gpio_is_valid_gpio((gpio_num_t)Pins::BUTTON_WEATHER_ONLY)) {
            ESP_LOGI(TAG, "✓ GPIO %d supports RTC", Pins::BUTTON_WEATHER_ONLY);
        } else {
            ESP_LOGE(TAG, "✗ GPIO %d does NOT support RTC!", Pins::BUTTON_WEATHER_ONLY);
        }
        if (rtc_gpio_is_valid_gpio((gpio_num_t)Pins::BUTTON_DEPARTURE_ONLY)) {
            ESP_LOGI(TAG, "✓ GPIO %d supports RTC", Pins::BUTTON_DEPARTURE_ONLY);
        } else {
            ESP_LOGE(TAG, "✗ GPIO %d does NOT support RTC!", Pins::BUTTON_DEPARTURE_ONLY);
        }
    } else {
        ESP_LOGI(TAG, "Button manager not available");
    }
}

int8_t ButtonManager::getWakeupButtonMode() {
    if (HAS_BUTTON) {
        if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT1) {
            return -1;
        }
        uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
        if (wakeup_pin_mask == 0) return -1;

        if (wakeup_pin_mask & (1ULL << Pins::BUTTON_HALF_AND_HALF)) {
            ESP_LOGI(TAG, "Woken by BUTTON_HALF_AND_HALF");
            return DISPLAY_MODE_HALF_AND_HALF;
        } else if (wakeup_pin_mask & (1ULL << Pins::BUTTON_WEATHER_ONLY)) {
            ESP_LOGI(TAG, "Woken by BUTTON_WEATHER_ONLY");
            return DISPLAY_MODE_WEATHER_ONLY;
        } else if (wakeup_pin_mask & (1ULL << Pins::BUTTON_DEPARTURE_ONLY)) {
            ESP_LOGI(TAG, "Woken by BUTTON_DEPARTURE_ONLY");
            return DISPLAY_MODE_TRANSPORT_ONLY;
        }
    }
    return -1;
}

void ButtonManager::enableButtonWakeup() {
    if (HAS_BUTTON) {
        uint64_t button_mask = getButtonMask();
        esp_err_t result = esp_sleep_enable_ext1_wakeup(button_mask, ESP_EXT1_WAKEUP_ANY_LOW);
        if (result == ESP_OK) {
            ESP_LOGI(TAG, "✓ EXT1 wakeup enabled (mask: 0x%llx)", button_mask);
        } else {
            ESP_LOGE(TAG, "✗ EXT1 wakeup failed: %s", esp_err_to_name(result));
        }
    }
}

uint64_t ButtonManager::getButtonMask() {
    if (HAS_BUTTON) {
        return (1ULL << Pins::BUTTON_HALF_AND_HALF) |
               (1ULL << Pins::BUTTON_WEATHER_ONLY) |
               (1ULL << Pins::BUTTON_DEPARTURE_ONLY);
    }
    return 0;
}

// =============================================================================
// handleWakeupMode() — seconds-based temporary mode
// =============================================================================
void ButtonManager::handleWakeupMode() {
    if (!HAS_BUTTON) return;

    RTCConfigData& config = ConfigManager::getConfig();
    time_t currentTime;
    time(&currentTime);

    // Determine if a new button press occurred (from EXT1, synthetic mode, or NVS pending)
    int8_t buttonMode = syntheticButtonMode;
    if (buttonMode >= 0) {
        ESP_LOGI(TAG, "Consuming synthetic button mode: %d", buttonMode);
        syntheticButtonMode = -1;
    } else {
        buttonMode = getWakeupButtonMode();
    }

    // Check for pending temp mode saved to NVS by checkAndRestartIfButtonPressed()
    if (buttonMode < 0) {
        Preferences prefs;
        if (prefs.begin("mystation", false)) {
            if (prefs.getBool("pendingTemp", false)) {
                buttonMode = (int8_t)prefs.getUChar("pendingTempMode", 0xFF);
                // Clear the pending flag
                prefs.putBool("pendingTemp", false);
                prefs.remove("pendingTempMode");
                ESP_LOGI(TAG, "Consumed pending temp mode from NVS: %d", buttonMode);
                if (buttonMode == (int8_t)0xFF) buttonMode = -1;
            }
            prefs.end();
        }
    }

    if (buttonMode >= 0) {
        // New button press — activate/reset temp mode
        ESP_LOGI(TAG, "Button press! Activating temp mode: %d", buttonMode);
        config.inTemporaryMode = true;
        config.temporaryDisplayMode = (uint8_t)buttonMode;
        config.temporaryModeActivationTime = (uint32_t)currentTime;

    } else if (config.inTemporaryMode) {
        // No new button press — check if awake-press needs stamping or if expired

        if (config.temporaryModeActivationTime == 0) {
            // ISR set inTemporaryMode during awake cycle but couldn't stamp time.
            // Stamp it now (first boot after esp_restart).
            config.temporaryModeActivationTime = (uint32_t)currentTime;
            ESP_LOGI(TAG, "Awake-press boot — stamped activation time");
        }

        int elapsed = (int)(currentTime - config.temporaryModeActivationTime);
        ESP_LOGI(TAG, "Temp mode elapsed: %d/%d s", elapsed, TEMP_MODE_ACTIVE_DURATION);

        if (elapsed >= (int)TEMP_MODE_ACTIVE_DURATION) {
            config.inTemporaryMode = false;
            config.temporaryDisplayMode = 0xFF;
            config.temporaryModeActivationTime = 0;
            ESP_LOGI(TAG, "Temp mode expired — reverting to configured mode");
        }
    }
}

// =============================================================================
// GPIO ISR handlers — IRAM-safe, minimal work.
// Only the FIRST press is recorded; subsequent triggers are ignored until consumed.
// =============================================================================
static void IRAM_ATTR isrButton1() { if (isrButtonPressed < 0) isrButtonPressed = DISPLAY_MODE_HALF_AND_HALF; }
static void IRAM_ATTR isrButton2() { if (isrButtonPressed < 0) isrButtonPressed = DISPLAY_MODE_WEATHER_ONLY; }
static void IRAM_ATTR isrButton3() { if (isrButtonPressed < 0) isrButtonPressed = DISPLAY_MODE_TRANSPORT_ONLY; }

void ButtonManager::attachRunningInterrupts() {
    if (!HAS_BUTTON) return;

    isrButtonPressed = -1;

    // De-init RTC GPIO for button pins — after deep sleep wakeup, pins may still
    // be in RTC mode which prevents normal GPIO interrupts from working.
    rtc_gpio_deinit((gpio_num_t)Pins::BUTTON_HALF_AND_HALF);
    rtc_gpio_deinit((gpio_num_t)Pins::BUTTON_WEATHER_ONLY);
    rtc_gpio_deinit((gpio_num_t)Pins::BUTTON_DEPARTURE_ONLY);

    // Re-configure as normal GPIO with pull-up
    pinMode(Pins::BUTTON_HALF_AND_HALF, INPUT_PULLUP);
    pinMode(Pins::BUTTON_WEATHER_ONLY, INPUT_PULLUP);
    pinMode(Pins::BUTTON_DEPARTURE_ONLY, INPUT_PULLUP);
    delay(10); // Allow pull-ups to stabilize

    attachInterrupt(digitalPinToInterrupt(Pins::BUTTON_HALF_AND_HALF), isrButton1, FALLING);
    attachInterrupt(digitalPinToInterrupt(Pins::BUTTON_WEATHER_ONLY),  isrButton2, FALLING);
    attachInterrupt(digitalPinToInterrupt(Pins::BUTTON_DEPARTURE_ONLY), isrButton3, FALLING);

    // Record attach time for debounce — ignore ISR triggers in the first 500ms
    isrAttachTime = millis();

    // Discard any ISR that fired during attachment (pin was already LOW)
    isrButtonPressed = -1;

    ESP_LOGI(TAG, "Running-mode interrupts attached (GPIO %d, %d, %d)",
             Pins::BUTTON_HALF_AND_HALF, Pins::BUTTON_WEATHER_ONLY, Pins::BUTTON_DEPARTURE_ONLY);
}

bool ButtonManager::checkAndRestartIfButtonPressed() {
    if (!HAS_BUTTON) return false;

    int8_t mode = isrButtonPressed;
    if (mode < 0) return false;

    // Debounce: ignore if triggered within 500ms of attaching interrupts
    if ((millis() - isrAttachTime) < 500) {
        isrButtonPressed = -1;
        return false;
    }

    ESP_LOGI(TAG, "Button pressed while awake (mode %d) — activating temp mode, restarting", mode);

    // Persist pending temp mode to NVS so it survives esp_restart()
    // (RTC_DATA_ATTR is lost on software reset, only preserved across deep sleep)
    Preferences prefs;
    if (prefs.begin("mystation", false)) {
        prefs.putBool("pendingTemp", true);
        prefs.putUChar("pendingTempMode", (uint8_t)mode);
        prefs.end();
        ESP_LOGI(TAG, "Saved pending temp mode %d to NVS", mode);
    } else {
        ESP_LOGE(TAG, "Failed to save pending temp mode to NVS");
    }

    detachInterrupt(digitalPinToInterrupt(Pins::BUTTON_HALF_AND_HALF));
    detachInterrupt(digitalPinToInterrupt(Pins::BUTTON_WEATHER_ONLY));
    detachInterrupt(digitalPinToInterrupt(Pins::BUTTON_DEPARTURE_ONLY));

    esp_restart();
    return true; // unreachable
}
