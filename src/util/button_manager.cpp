#include "util/button_manager.h"
#include "build_config.h"
#include "config/pins.h"
#include "config/config_manager.h"
#include "util/timing_manager.h"
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

// Step the browsed day by +1/-1, wrapping circularly within the active browse
// context. WEATHER browse skips day 0 (range 1..max-1); SOLAR browse includes
// day 0 (range 0..max-1). maxDays = availableForecastDays (>=2), fallback 7.
static int8_t stepBrowseDay(RTCConfigData& config, int delta) {
    int8_t maxDays = config.availableForecastDays > 1 ? config.availableForecastDays : 7;
    int8_t lower = (config.browseContext == BROWSE_SOLAR) ? 0 : 1;
    int8_t upper = maxDays - 1;                 // last browsable future day
    if (upper < lower) upper = lower;           // degenerate (few-day models)

    int span = upper - lower + 1;               // number of days in the cycle
    int8_t cur = config.selectedForecastDay;
    if (cur < lower) cur = lower;
    if (cur > upper) cur = upper;

    // Circular step within [lower, upper].
    int idx = (cur - lower + delta) % span;
    if (idx < 0) idx += span;
    return (int8_t)(lower + idx);
}

// =============================================================================
// Weather-browse state machine (shared by both button paths)
// =============================================================================
// Decides how a button press is interpreted while the Weather-Full view is
// effectively showing (configured Weather-Only, OR Half&Half currently rendering
// weather-full — temporary or the outside-transport-hours fallback).
//
// Rules (weather browse only; no solar):
//   Not browsing (config.inWeatherBrowse == false):
//     B2 -> ENTER browse at day +1  (BROWSE_ENTER_OR_STEP)
//     B1 / B3 -> not a browse action (BROWSE_NONE) — caller does normal switch
//   Browsing (config.inWeatherBrowse == true):
//     B2 -> +1 day (wraps)          (BROWSE_ENTER_OR_STEP)
//     B3 -> -1 day (wraps)          (BROWSE_ENTER_OR_STEP)
//     B1 -> EXIT browse             (BROWSE_EXIT)
//
// Guard: only acts when getEffectiveDisplayMode() == WEATHER_ONLY and the press
// isn't APPLICATION_INFO. Mutates config.selectedForecastDay / browseContext /
// inWeatherBrowse. Returns how the caller should handle temp-mode bookkeeping.
BrowseResult ButtonManager::applyWeatherBrowsePress(RTCConfigData& config, int8_t buttonMode) {
    uint8_t effectiveMode = TimingManager::getEffectiveDisplayMode();
    if (effectiveMode != DISPLAY_MODE_WEATHER_ONLY
        || buttonMode == DISPLAY_MODE_APPLICATION_INFO) {
        return BROWSE_NONE;
    }

    if (!config.inWeatherBrowse) {
        // Not browsing yet: only B2 enters browse.
        if (buttonMode == DISPLAY_MODE_WEATHER_ONLY) {
            config.inWeatherBrowse = true;
            config.browseContext = BROWSE_WEATHER;
            config.selectedForecastDay = 1; // day +1 (tomorrow)
            ESP_LOGI(TAG, "Weather browse: ENTER -> day %d", config.selectedForecastDay);
            return BROWSE_ENTER_OR_STEP;
        }
        return BROWSE_NONE; // B1/B3 fall through to normal mode switching
    }

    // Already browsing.
    if (buttonMode == DISPLAY_MODE_WEATHER_ONLY) {        // B2: next day
        config.selectedForecastDay = stepBrowseDay(config, +1);
        ESP_LOGI(TAG, "Weather browse: +1 -> day %d", config.selectedForecastDay);
        return BROWSE_ENTER_OR_STEP;
    } else if (buttonMode == DISPLAY_MODE_TRANSPORT_ONLY) { // B3: previous day
        config.selectedForecastDay = stepBrowseDay(config, -1);
        ESP_LOGI(TAG, "Weather browse: -1 -> day %d", config.selectedForecastDay);
        return BROWSE_ENTER_OR_STEP;
    } else { // B1: exit browse
        config.inWeatherBrowse = false;
        config.selectedForecastDay = 0;
        config.browseContext = BROWSE_WEATHER;
        ESP_LOGI(TAG, "Weather browse: EXIT -> configured mode");
        return BROWSE_EXIT;
    }
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
    bool pendingApplied = false; // true if awake-press already decided browse state (via NVS)
    if (buttonMode >= 0) {
        ESP_LOGI(TAG, "Consuming synthetic button mode: %d", buttonMode);
        syntheticButtonMode = -1;
    } else {
        buttonMode = getWakeupButtonMode();
    }

    // Check for pending temp mode saved to NVS by checkAndRestartIfButtonPressed().
    // The awake path already ran the browse state machine, so we just restore the
    // resulting state here and skip re-interpreting the press.
    if (buttonMode < 0) {
        Preferences prefs;
        if (prefs.begin("mystation", false)) {
            if (prefs.getBool("pendingTemp", false)) {
                buttonMode = (int8_t)prefs.getUChar("pendingTempMode", 0xFF);
                config.selectedForecastDay = prefs.getChar("pendingDay", 0);
                config.browseContext = prefs.getUChar("pendingCtx", BROWSE_WEATHER);
                config.inWeatherBrowse = prefs.getBool("pendingBrowse", false);
                pendingApplied = true; // awake-press already computed browse state
                prefs.putBool("pendingTemp", false);
                prefs.remove("pendingTempMode");
                prefs.remove("pendingDay");
                prefs.remove("pendingCtx");
                prefs.remove("pendingBrowse");
                ESP_LOGI(TAG, "Consumed pending temp mode from NVS: %d, day %d, ctx %d, browse %d",
                         buttonMode, config.selectedForecastDay, config.browseContext,
                         config.inWeatherBrowse);
                if (buttonMode == (int8_t)0xFF) buttonMode = -1;
            }
            prefs.end();
        }
    }

    if (buttonMode >= 0) {
        // Run the shared weather-browse state machine first (unless the awake path
        // already decided it and persisted the result).
        BrowseResult br = pendingApplied ? BROWSE_NONE
                                         : applyWeatherBrowsePress(config, buttonMode);

        if (pendingApplied) {
            // Awake-press result already in config. Apply temp-mode bookkeeping based
            // on the restored state: browsing -> keep Weather-Full; otherwise the
            // persisted buttonMode is the temp mode to show.
            if (config.inWeatherBrowse) {
                config.inTemporaryMode = true;
                config.temporaryDisplayMode = DISPLAY_MODE_WEATHER_ONLY;
                config.temporaryModeActivationTime = (uint32_t)currentTime;
            } else {
                config.inTemporaryMode = true;
                config.temporaryDisplayMode = (uint8_t)buttonMode;
                config.temporaryModeActivationTime = (uint32_t)currentTime;
            }
        } else if (br == BROWSE_ENTER_OR_STEP) {
            // Entered browse or stepped a day — keep a temporary Weather-Full so the
            // browse view stays on screen.
            config.inTemporaryMode = true;
            config.temporaryDisplayMode = DISPLAY_MODE_WEATHER_ONLY;
            config.temporaryModeActivationTime = (uint32_t)currentTime;
        } else if (br == BROWSE_EXIT) {
            // Exited browse — clear temporary mode so the device shows the CONFIGURED
            // mode (getEffectiveDisplayMode resolves Half&Half to split or weather-full
            // by transport time).
            config.inTemporaryMode = false;
            config.temporaryDisplayMode = 0xFF;
            config.temporaryModeActivationTime = 0;
            ESP_LOGI(TAG, "Weather browse exit -> configured mode");
        } else {
            // BROWSE_NONE — normal mode switching (B1/B2/B3 as a temporary mode).
            ESP_LOGI(TAG, "Button press! Activating temp mode: %d", buttonMode);
            config.inTemporaryMode = true;
            config.temporaryDisplayMode = (uint8_t)buttonMode;
            config.temporaryModeActivationTime = (uint32_t)currentTime;
        }

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
            config.selectedForecastDay = 0;
            config.browseContext = BROWSE_WEATHER;
            config.inWeatherBrowse = false;
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

    RTCConfigData& config = ConfigManager::getConfig();

    // Run the shared weather-browse state machine. It decides enter/step/exit and
    // mutates config browse state (selectedForecastDay, browseContext, inWeatherBrowse).
    BrowseResult br = applyWeatherBrowsePress(config, mode);

    // Decide what temporary mode to show after the restart.
    uint8_t persistMode = (uint8_t)mode;
    bool skipPersist = false;
    if (br == BROWSE_ENTER_OR_STEP) {
        persistMode = DISPLAY_MODE_WEATHER_ONLY; // keep Weather-Full while browsing
    } else if (br == BROWSE_EXIT) {
        // Exit browse -> revert to the configured mode. esp_restart() keeps RTC, so
        // clear the temp fields explicitly AND skip persisting a pending temp mode.
        config.inTemporaryMode = false;
        config.temporaryDisplayMode = 0xFF;
        config.temporaryModeActivationTime = 0;
        skipPersist = true;
    }
    // BROWSE_NONE: normal mode switch -> persist the pressed button as the temp mode.

    if (skipPersist) {
        Preferences prefs;
        if (prefs.begin("mystation", false)) {
            prefs.putBool("pendingTemp", false);
            prefs.remove("pendingTempMode");
            prefs.remove("pendingDay");
            prefs.remove("pendingCtx");
            prefs.remove("pendingBrowse");
            prefs.end();
        }
        ESP_LOGI(TAG, "Weather browse EXIT (awake) — cleared pending, reverting to configured mode");
    } else {
        // Persist pending temp mode to NVS so it survives esp_restart().
        Preferences prefs;
        if (prefs.begin("mystation", false)) {
            prefs.putBool("pendingTemp", true);
            prefs.putUChar("pendingTempMode", persistMode);
            prefs.putChar("pendingDay", config.selectedForecastDay);
            prefs.putUChar("pendingCtx", config.browseContext);
            prefs.putBool("pendingBrowse", config.inWeatherBrowse);
            prefs.end();
            ESP_LOGI(TAG, "Saved pending temp mode %d, day %d, ctx %d, browse %d to NVS",
                     persistMode, config.selectedForecastDay, config.browseContext,
                     config.inWeatherBrowse);
        } else {
            ESP_LOGE(TAG, "Failed to save pending temp mode to NVS");
        }
    }

    detachInterrupt(digitalPinToInterrupt(Pins::BUTTON_HALF_AND_HALF));
    detachInterrupt(digitalPinToInterrupt(Pins::BUTTON_WEATHER_ONLY));
    detachInterrupt(digitalPinToInterrupt(Pins::BUTTON_DEPARTURE_ONLY));

    esp_restart();
    return true; // unreachable
}
