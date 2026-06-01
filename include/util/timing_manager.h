#pragma once

#include <Arduino.h>

class TimingManager {
public:
    // Get next sleep duration based on mode and next required update
    static uint64_t getNextSleepDurationSeconds();

    // Check if transport should be active at current time
    static bool isTransportActiveTime();

    // Check if we're in weekend mode
    static bool isWeekend();

    // Update last refresh timestamps
    static void markWeatherUpdated();
    static void markTransportUpdated();

    // Check if it's time for a specific update type
    static bool isTimeForWeatherUpdate();

    // Get effective display mode (considers temporary mode)
    static uint8_t getEffectiveDisplayMode();

    // RTC timestamp management (public for testing)
    static uint32_t getLastWeatherUpdate();
    static uint32_t getLastTransportUpdate();
    static void setLastWeatherUpdate(uint32_t timestamp);
    static void setLastTransportUpdate(uint32_t timestamp);
    // OTA update timestamp management (public for testing)
    static uint32_t getLastOTACheck();
    static void setLastOTACheck(uint32_t timestamp);

private:
    // Time utility helpers
    static int parseTimeString(const String& timeStr);
    static bool isTimeInRange(uint32_t currentMinutes, uint32_t startMinutes, uint32_t endMinutes);
    static uint16_t getCurrentMin();
    static uint16_t getSleepStartMin();
    static uint16_t getSleepEndMin();
    static bool isWeekend(time_t timestamp);
    static bool isInDeepSleepPeriod();
    static bool isInDeepSleepPeriod(uint32_t timestamp);

    // Sleep duration calculation helpers
    static uint32_t calculateNextOTACheckTime(uint32_t currentTimeSeconds);
    static uint32_t calculateNextWeatherUpdate(uint32_t currentTimeSeconds);
    static uint32_t calculateNextTransportUpdate(uint32_t currentTimeSeconds);
    static uint32_t adjustForDeepSleepPeriod(uint32_t nearestUpdate, bool isOTAUpdate);

    // Transport active hours helpers
    static bool isTransportActiveAtTime(uint32_t timestamp);
    static uint32_t calculateNextActiveTransportTime(uint32_t currentTime);
};
