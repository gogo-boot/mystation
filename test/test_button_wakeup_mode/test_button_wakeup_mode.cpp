/**
 * test_button_wakeup_mode.cpp
 *
 * Tests for the seconds-based temp mode state machine in ButtonManager.
 * Temp mode expires after TEMP_MODE_ACTIVE_DURATION (120s) wall-clock time.
 */

#include <unity.h>
#include <ctime>
#include <cstring>

#include "esp32_mocks.h"
#include "esp_sleep.h"
#include "config/config_manager.h"
#include "config/config_struct.h"

constexpr uint32_t TEMP_MODE_ACTIVE_DURATION = 120;

// Simulate ISR press (awake): sets inTemporaryMode, activationTime=0 (sentinel)
void simulateISRPress(uint8_t mode) {
    RTCConfigData& config = ConfigManager::getConfig();
    config.inTemporaryMode = true;
    config.temporaryDisplayMode = mode;
    config.temporaryModeActivationTime = 0;
}

// Simulate handleWakeupMode()
void simulateHandleWakeupMode(int8_t ext1Mode, int8_t synthetic, time_t now) {
    RTCConfigData& config = ConfigManager::getConfig();

    int8_t buttonMode = (synthetic >= 0) ? synthetic : ext1Mode;

    if (buttonMode >= 0) {
        config.inTemporaryMode = true;
        config.temporaryDisplayMode = (uint8_t)buttonMode;
        config.temporaryModeActivationTime = (uint32_t)now;
    } else if (config.inTemporaryMode) {
        if (config.temporaryModeActivationTime == 0) {
            config.temporaryModeActivationTime = (uint32_t)now;
        }
        int elapsed = (int)(now - (time_t)config.temporaryModeActivationTime);
        if (elapsed >= (int)TEMP_MODE_ACTIVE_DURATION) {
            config.inTemporaryMode = false;
            config.temporaryDisplayMode = 0xFF;
            config.temporaryModeActivationTime = 0;
        }
    }
}

void resetConfig() {
    RTCConfigData& config = ConfigManager::getConfig();
    config.inTemporaryMode = false;
    config.temporaryDisplayMode = 0xFF;
    config.temporaryModeActivationTime = 0;
    config.displayMode = DISPLAY_MODE_HALF_AND_HALF;
}

// ── Tests ───────────────────────────────────────────────────────────────

void test_ext1_wakeup_activates_temp_mode() {
    resetConfig();
    simulateHandleWakeupMode(DISPLAY_MODE_WEATHER_ONLY, -1, 1000000);
    RTCConfigData& c = ConfigManager::getConfig();
    TEST_ASSERT_TRUE(c.inTemporaryMode);
    TEST_ASSERT_EQUAL(DISPLAY_MODE_WEATHER_ONLY, c.temporaryDisplayMode);
    TEST_ASSERT_EQUAL((uint32_t)1000000, c.temporaryModeActivationTime);
}

void test_awake_press_stamps_time_on_restart() {
    resetConfig();
    simulateISRPress(DISPLAY_MODE_TRANSPORT_ONLY);
    simulateHandleWakeupMode(-1, -1, 1000000);
    RTCConfigData& c = ConfigManager::getConfig();
    TEST_ASSERT_TRUE(c.inTemporaryMode);
    TEST_ASSERT_EQUAL(DISPLAY_MODE_TRANSPORT_ONLY, c.temporaryDisplayMode);
    TEST_ASSERT_EQUAL((uint32_t)1000000, c.temporaryModeActivationTime);
}

void test_temp_mode_active_before_120s() {
    resetConfig();
    simulateHandleWakeupMode(DISPLAY_MODE_WEATHER_ONLY, -1, 1000000);
    simulateHandleWakeupMode(-1, -1, 1000119);
    TEST_ASSERT_TRUE(ConfigManager::getConfig().inTemporaryMode);
}

void test_temp_mode_expires_at_120s() {
    resetConfig();
    simulateHandleWakeupMode(DISPLAY_MODE_WEATHER_ONLY, -1, 1000000);
    simulateHandleWakeupMode(-1, -1, 1000120);
    TEST_ASSERT_FALSE(ConfigManager::getConfig().inTemporaryMode);
    TEST_ASSERT_EQUAL(0xFF, ConfigManager::getConfig().temporaryDisplayMode);
}

void test_new_press_resets_timer() {
    resetConfig();
    simulateHandleWakeupMode(DISPLAY_MODE_WEATHER_ONLY, -1, 1000000);
    simulateHandleWakeupMode(DISPLAY_MODE_TRANSPORT_ONLY, -1, 1000100);
    RTCConfigData& c = ConfigManager::getConfig();
    TEST_ASSERT_TRUE(c.inTemporaryMode);
    TEST_ASSERT_EQUAL(DISPLAY_MODE_TRANSPORT_ONLY, c.temporaryDisplayMode);
    TEST_ASSERT_EQUAL((uint32_t)1000100, c.temporaryModeActivationTime);
    simulateHandleWakeupMode(-1, -1, 1000219);
    TEST_ASSERT_TRUE(ConfigManager::getConfig().inTemporaryMode);
    simulateHandleWakeupMode(-1, -1, 1000220);
    TEST_ASSERT_FALSE(ConfigManager::getConfig().inTemporaryMode);
}

void test_awake_press_expires_after_120s() {
    resetConfig();
    simulateISRPress(DISPLAY_MODE_WEATHER_ONLY);
    time_t restartTime = 1000000;
    simulateHandleWakeupMode(-1, -1, restartTime);
    simulateHandleWakeupMode(-1, -1, restartTime + 120);
    TEST_ASSERT_FALSE(ConfigManager::getConfig().inTemporaryMode);
}

// ── Main ────────────────────────────────────────────────────────────────
void setUp()    { resetConfig(); }
void tearDown() {}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_ext1_wakeup_activates_temp_mode);
    RUN_TEST(test_awake_press_stamps_time_on_restart);
    RUN_TEST(test_temp_mode_active_before_120s);
    RUN_TEST(test_temp_mode_expires_at_120s);
    RUN_TEST(test_new_press_resets_timer);
    RUN_TEST(test_awake_press_expires_after_120s);
    return UNITY_END();
}
