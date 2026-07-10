#include "util/timing_manager.h"
#ifdef NATIVE_TEST
#include "time_manager.h"
#include "esp_log.h"
#include "mock_time.h"
#define GET_CURRENT_TIME() MockTime::now()
#else
#include "util/time_manager.h"
#include <esp_mac.h>
#define GET_CURRENT_TIME() ({ time_t t; time(&t); t; })
#endif
#include <time.h>
#include "config/config_manager.h"

static const char* TAG = "TIMING_MGR";

// RTC memory for storing last update timestamps
RTC_DATA_ATTR uint32_t lastWeatherUpdate = 0;
RTC_DATA_ATTR uint32_t lastTransportUpdate = 0;
RTC_DATA_ATTR uint32_t lastOTACheck = 0;

// ============================================================================
// Device Jitter — deterministic per-device offset to spread API requests
// ============================================================================

uint32_t TimingManager::getDeviceJitterSeed() {
#ifdef NATIVE_TEST
    // Fixed seed for reproducible test results
    return 0xDEADBEEF;
#else
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    // Simple hash of MAC address — deterministic and unique per device
    return (uint32_t)(mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5];
#endif
}

// ============================================================================
// Helper Functions for Sleep Duration Calculation
// ============================================================================


uint32_t TimingManager::calculateNextWeatherUpdate(uint32_t currentTimeSeconds) {
    RTCConfigData& config = ConfigManager::getConfig();
    uint32_t lastUpdate = getLastWeatherUpdate();
    uint32_t intervalSeconds = config.weatherInterval * 3600; // hours to seconds

    uint32_t nextUpdate = (lastUpdate == 0) ? currentTimeSeconds : lastUpdate + intervalSeconds;

    // If update is overdue (in the past), return current time for immediate update
    if (nextUpdate < currentTimeSeconds) {
        ESP_LOGI(TAG, "Weather update overdue - next update: NOW (was scheduled for: %u)", nextUpdate);
        nextUpdate = currentTimeSeconds;
    }

    ESP_LOGI(TAG, "Weather interval: %u hours (%u seconds), Next weather update: %u",
             config.weatherInterval, intervalSeconds, nextUpdate);

    return nextUpdate;
}

uint32_t TimingManager::calculateNextTransportUpdate(uint32_t currentTimeSeconds) {
    RTCConfigData& config = ConfigManager::getConfig();
    uint32_t lastUpdate = getLastTransportUpdate();
    uint32_t intervalSeconds = config.transportInterval * 60; // minutes to seconds

    uint32_t nextUpdate = (lastUpdate == 0) ? currentTimeSeconds + intervalSeconds : lastUpdate + intervalSeconds;

    // If update is overdue (in the past), return current time for immediate update
    if (nextUpdate < currentTimeSeconds) {
        ESP_LOGI(TAG, "Transport update overdue - next update: NOW (was scheduled for: %u)", nextUpdate);
        nextUpdate = currentTimeSeconds;
    }

    ESP_LOGI(TAG, "Departure interval: %u minutes (%u seconds), Next departure update: %u",
             config.transportInterval, intervalSeconds, nextUpdate);

    return nextUpdate;
}

bool TimingManager::isTransportActiveAtTime(uint32_t timestamp) {
    RTCConfigData& config = ConfigManager::getConfig();

    tm timeInfo;
    time_t time = (time_t)timestamp;
    localtime_r(&time, &timeInfo);

    bool weekend = isWeekend(timestamp);

    String start = weekend ? String(config.weekendTransportStart) : String(config.transportActiveStart);
    String end = weekend ? String(config.weekendTransportEnd) : String(config.transportActiveEnd);

    int minutes = timeInfo.tm_hour * 60 + timeInfo.tm_min;
    int startMin = parseTimeString(start);
    int endMin = parseTimeString(end);

    bool isActive = isTimeInRange(minutes, startMin, endMin);

    // Special case: boundary check - treat end boundary as inactive if no previous update
    if (isActive && minutes == endMin && getLastTransportUpdate() == 0) {
        ESP_LOGD(TAG, "At end boundary of active hours with no previous update - treating as inactive");
        return false;
    }

    return isActive;
}

uint32_t TimingManager::calculateNextActiveTransportTime(uint32_t currentTime) {
    RTCConfigData& config = ConfigManager::getConfig();

    tm currentTm;
    time_t now = (time_t)currentTime;
    localtime_r(&now, &currentTm);

    int currentMin = currentTm.tm_hour * 60 + currentTm.tm_min;
    bool isCurrentWeekend = config.weekendMode && (currentTm.tm_wday == 0 || currentTm.tm_wday == 6);

    String start = isCurrentWeekend ? String(config.weekendTransportStart) : String(config.transportActiveStart);
    int startMin = parseTimeString(start);

    uint32_t nextActiveTime;

    if (currentMin < startMin) {
        // Active period starts later today
        int minutesUntil = startMin - currentMin;
        nextActiveTime = currentTime + (minutesUntil * 60);
        ESP_LOGD(TAG, "Transport active starts in %d minutes", minutesUntil);
    } else {
        // Active period starts tomorrow
        int minutesToMidnight = (24 * 60) - currentMin;

        // Check if tomorrow is weekend
        int nextDayOfWeek = (currentTm.tm_wday + 1) % 7;
        bool isTomorrowWeekend = config.weekendMode && (nextDayOfWeek == 0 || nextDayOfWeek == 6);
        String tomorrowStart = isTomorrowWeekend
                                   ? String(config.weekendTransportStart)
                                   : String(config.transportActiveStart);
        int tomorrowStartMin = parseTimeString(tomorrowStart);

        nextActiveTime = currentTime + ((minutesToMidnight + tomorrowStartMin) * 60);
        ESP_LOGD(TAG, "Transport active starts tomorrow at %02d:%02d", tomorrowStartMin / 60, tomorrowStartMin % 60);
    }

    ESP_LOGI(TAG, "Next transport active time: %u", nextActiveTime);
    return nextActiveTime;
}

uint32_t TimingManager::adjustForDeepSleepPeriod(uint32_t nearestUpdate, bool isOTAUpdate) {
    RTCConfigData& config = ConfigManager::getConfig();

    // Get update time info
    tm updateTimeInfo;
    time_t updateTime = (time_t)nearestUpdate;
    localtime_r(&updateTime, &updateTimeInfo);
    uint16_t updateMinutes = updateTimeInfo.tm_hour * 60 + updateTimeInfo.tm_min;

    uint16_t sleepStartMin = getSleepStartMin();
    uint16_t sleepEndMin = getSleepEndMin();

    // Check if update is in sleep period
    if (!isInDeepSleepPeriod(nearestUpdate)) {
        return nearestUpdate; // Not in sleep period
    }

    ESP_LOGI(TAG, "Next update (%d:%02d) falls within sleep period (%d - %d)",
             updateTimeInfo.tm_hour, updateTimeInfo.tm_min, sleepStartMin, sleepEndMin);

    // OTA updates bypass sleep period
    if (isOTAUpdate) {
        ESP_LOGI(TAG, "OTA update scheduled during sleep period - bypassing sleep restrictions");
        return nearestUpdate;
    }

    // Calculate sleep end time
    uint32_t sleepEndSeconds;
    if (sleepEndMin > updateMinutes) {
        sleepEndSeconds = nearestUpdate + ((sleepEndMin - updateMinutes) * 60);
    } else {
        int minutesUntilNextDay = (24 * 60) - updateMinutes;
        sleepEndSeconds = nearestUpdate + ((minutesUntilNextDay + sleepEndMin) * 60);
    }

    // Check if sleep crosses weekend boundary
    tm sleepEndTimeInfo;
    time_t sleepEndTime = (time_t)sleepEndSeconds;
    localtime_r(&sleepEndTime, &sleepEndTimeInfo);

    // Determine if update time is weekend
    bool isUpdateWeekend = isWeekend(updateTime);
    bool isSleepEndWeekend = isWeekend(sleepEndTime);

    if (isSleepEndWeekend != isUpdateWeekend) {
        ESP_LOGI(TAG, "Sleep crosses weekend boundary - adjusting sleep end time");

        String correctSleepEnd = isSleepEndWeekend ? String(config.weekendSleepEnd) : String(config.sleepEnd);
        int correctSleepEndMin = parseTimeString(correctSleepEnd);

        if (correctSleepEndMin > updateMinutes) {
            sleepEndSeconds = nearestUpdate + ((correctSleepEndMin - updateMinutes) * 60);
        } else {
            int minutesUntilNextDay = (24 * 60) - updateMinutes;
            sleepEndSeconds = nearestUpdate + ((minutesUntilNextDay + correctSleepEndMin) * 60);
        }

        ESP_LOGI(TAG, "Adjusted sleep end to %s time: %u seconds",
                 isSleepEndWeekend ? "weekend" : "weekday", sleepEndSeconds);
    }

    ESP_LOGI(TAG, "Final wake time: %u seconds", sleepEndSeconds);
    return sleepEndSeconds;
}

// ============================================================================
// Main Sleep Duration Calculation — Rule-based priority queue
//
// Each "rule" proposes a wake-up time. We collect all candidates, apply the
// sleep window filter (push non-OTA candidates past the sleep end), then
// pick the earliest.
// ============================================================================

static constexpr int MAX_CANDIDATES = 5;
static constexpr uint64_t MINIMUM_SLEEP_SECONDS = 30;

struct WakeCandidate {
    uint32_t time;         // Absolute timestamp (seconds since epoch)
    bool bypassesSleep;    // If true, ignores the sleep window
    const char* reason;    // For logging
};

uint64_t TimingManager::getNextSleepDurationSeconds() {
    time_t now = GET_CURRENT_TIME();
    uint32_t currentTime = (uint32_t)now;
    RTCConfigData& config = ConfigManager::getConfig();

    ESP_LOGI(TAG, "Sleep calc - mode: %d (configured: %d, temp: %d), time: %u",
             getEffectiveDisplayMode(), config.displayMode, config.inTemporaryMode, currentTime);

    // ── TEMPORARY MODE (early return — short-lived override) ──────────────
    if (config.inTemporaryMode) {
        int elapsed = currentTime - config.temporaryModeActivationTime;
        const int TEMP_MODE_DURATION = 120; // 2 minutes
        int remaining = TEMP_MODE_DURATION - elapsed;

        if (isInDeepSleepPeriod()) {
            // In sleep window → sleep until sleep window ends
            int currentMin = getCurrentMin();
            int sleepEndMin = getSleepEndMin();
            int minutesUntilEnd = (sleepEndMin > currentMin)
                ? (sleepEndMin - currentMin)
                : ((24 * 60) - currentMin + sleepEndMin);
            return (uint64_t)max((int)MINIMUM_SLEEP_SECONDS, minutesUntilEnd * 60);
        }

        if (remaining > 0) {
            return (uint64_t)max((int)MINIMUM_SLEEP_SECONDS, remaining);
        }

        // Temp mode expired — fall through to normal calculation
        ESP_LOGW(TAG, "Temp mode expired but flag not cleared — falling through");
    }

    // ── COLLECT WAKE CANDIDATES ──────────────────────────────────────────
    WakeCandidate candidates[MAX_CANDIDATES];
    int count = 0;
    uint8_t effectiveMode = getEffectiveDisplayMode();

    // Rule 1: Weather update (all modes that show weather)
    if (effectiveMode != DISPLAY_MODE_TRANSPORT_ONLY) {
        uint32_t nextWeather = calculateNextWeatherUpdate(currentTime);
        if (nextWeather > 0) {
            candidates[count++] = {nextWeather, false, "weather-update"};
        }
    }

    // Rule 2: Transport update (only when inside transport window)
    if (effectiveMode == DISPLAY_MODE_HALF_AND_HALF || effectiveMode == DISPLAY_MODE_TRANSPORT_ONLY) {
        uint32_t nextTransport = calculateNextTransportUpdate(currentTime);
        if (nextTransport > 0 && isTransportActiveAtTime(nextTransport)) {
            candidates[count++] = {nextTransport, false, "transport-update"};
        }
    }

    // Rule 3: Transport window start (wake to begin showing departures)
    // Applies when configured as half&half or transport-only, but currently outside transport window
    if ((config.displayMode == DISPLAY_MODE_HALF_AND_HALF ||
         config.displayMode == DISPLAY_MODE_TRANSPORT_ONLY) && !isTransportActiveTime()) {
        uint32_t nextStart = calculateNextActiveTransportTime(currentTime);
        if (nextStart > currentTime) {
            candidates[count++] = {nextStart, false, "transport-window-start"};
        }
    }

    // Rule 4: OTA check (bypasses sleep window)
    if (config.otaEnabled) {
        uint32_t nextOTA = calculateNextOTACheckTime(currentTime);
        if (nextOTA > currentTime) {
            candidates[count++] = {nextOTA, true, "ota-check"};
        }
    }

    // ── CHECK FOR OVERDUE CANDIDATES (wake immediately) ─────────────────
    for (int i = 0; i < count; i++) {
        if (candidates[i].time <= currentTime) {
            ESP_LOGI(TAG, "Candidate '%s' is overdue — wake immediately", candidates[i].reason);
            return MINIMUM_SLEEP_SECONDS;
        }
    }

    // ── APPLY SLEEP WINDOW FILTER ────────────────────────────────────────
    // Non-OTA candidates that fall inside the sleep window are pushed to sleep end
    for (int i = 0; i < count; i++) {
        if (!candidates[i].bypassesSleep) {
            candidates[i].time = adjustForDeepSleepPeriod(candidates[i].time, false);
        }
    }

    // ── PICK EARLIEST ────────────────────────────────────────────────────
    uint32_t earliest = 0;
    const char* earliestReason = "none";
    for (int i = 0; i < count; i++) {
        if (earliest == 0 || candidates[i].time < earliest) {
            earliest = candidates[i].time;
            earliestReason = candidates[i].reason;
        }
    }

    // ── CALCULATE DURATION ───────────────────────────────────────────────
    uint64_t sleepSeconds;
    if (earliest > currentTime) {
        sleepSeconds = (uint64_t)(earliest - currentTime);
    } else {
        sleepSeconds = MINIMUM_SLEEP_SECONDS;
    }

    if (sleepSeconds < MINIMUM_SLEEP_SECONDS) {
        sleepSeconds = MINIMUM_SLEEP_SECONDS;
    }

    // ── APPLY DEVICE-SPECIFIC JITTER ─────────────────────────────────────
    // Add a deterministic offset (0 to MAX_JITTER_SECONDS) based on the device's
    // MAC address. Only applied on the first wake after boot (when no updates have
    // been recorded yet). This spreads the initial API requests evenly across devices
    // that boot simultaneously (thundering herd prevention) without drifting the
    // update interval on subsequent cycles.
    if (getLastWeatherUpdate() == 0 && getLastTransportUpdate() == 0) {
        uint32_t jitter = getDeviceJitterSeed() % MAX_JITTER_SECONDS;
        sleepSeconds += jitter;
        ESP_LOGI(TAG, "First wake after boot — applying jitter: +%u s", jitter);
    }

    ESP_LOGI(TAG, "Next wake: %s at %u (in %llu seconds / %llu min)",
             earliestReason, earliest, sleepSeconds, sleepSeconds / 60);

    return sleepSeconds;
}

bool TimingManager::isTransportActiveTime() {
    int currentMinutes = getCurrentMin();

    String activeStart, activeEnd;
    if (isWeekend()) {
        activeStart = ConfigManager::getWeekendTransportStart();
        activeEnd = ConfigManager::getWeekendTransportEnd();
    } else {
        activeStart = ConfigManager::getTransportActiveStart();
        activeEnd = ConfigManager::getTransportActiveEnd();
    }

    int startMinutes = parseTimeString(activeStart);
    int endMinutes = parseTimeString(activeEnd);

    return isTimeInRange(currentMinutes, startMinutes, endMinutes);
}

uint16_t TimingManager::getCurrentMin() {
    time_t now = GET_CURRENT_TIME();
    tm timeInfo;
    localtime_r(&now, &timeInfo);
    return timeInfo.tm_hour * 60 + timeInfo.tm_min;
}

uint16_t TimingManager::getSleepStartMin() {
    RTCConfigData& config = ConfigManager::getConfig();
    String sleepStart = isWeekend() ? String(config.weekendSleepStart) : String(config.sleepStart);
    return parseTimeString(sleepStart);
}

uint16_t TimingManager::getSleepEndMin() {
    RTCConfigData& config = ConfigManager::getConfig();
    String sleepEnd = isWeekend() ? String(config.weekendSleepEnd) : String(config.sleepEnd);
    return parseTimeString(sleepEnd);
}

bool TimingManager::isInDeepSleepPeriod() {
    return isTimeInRange(getCurrentMin(), getSleepStartMin(), getSleepEndMin());
}


bool TimingManager::isInDeepSleepPeriod(uint32_t timestamp) {
    tm timeInfo;
    time_t time = (time_t)timestamp;
    localtime_r(&time, &timeInfo);
    uint32_t updateMinutes = timeInfo.tm_hour * 60 + timeInfo.tm_min;
    return isTimeInRange(updateMinutes, getSleepStartMin(), getSleepEndMin());
}

bool TimingManager::isWeekend() {
    return isWeekend(GET_CURRENT_TIME());
}

bool TimingManager::isWeekend(time_t timestamp) {
    RTCConfigData& config = ConfigManager::getConfig();
    if (!config.weekendMode) {
        return false;
    }
    tm timeInfo;
    localtime_r(&timestamp, &timeInfo);

    // tm_wday: 0 = Sunday, 6 = Saturday
    return (timeInfo.tm_wday == 0 || timeInfo.tm_wday == 6);
}

void TimingManager::markWeatherUpdated() {
    time_t now = GET_CURRENT_TIME();
    setLastWeatherUpdate((uint32_t)now);
    ESP_LOGI(TAG, "Weather update timestamp recorded: %u", (uint32_t)now);
}

void TimingManager::markTransportUpdated() {
    time_t now = GET_CURRENT_TIME();
    setLastTransportUpdate((uint32_t)now);
    ESP_LOGI(TAG, "Tranport update timestamp recorded: %u", (uint32_t)now);
}

bool TimingManager::isTimeForWeatherUpdate() {
    RTCConfigData& config = ConfigManager::getConfig();
    uint32_t lastUpdate = getLastWeatherUpdate();

    if (lastUpdate == 0) {
        ESP_LOGI(TAG, "No previous weather update - update required");
        return true; // No previous update
    }

    time_t now = GET_CURRENT_TIME();
    uint32_t currentTime = (uint32_t)now;

    uint8_t tolerance = 20; // seconds
    uint32_t intervalSeconds = config.weatherInterval * 3600 - tolerance; // Convert hours to seconds
    bool needUpdate = (currentTime - lastUpdate) > intervalSeconds;

    ESP_LOGI(TAG, "Weather: last=%u, now=%u, interval=%u hours, need_update=%s",
             lastUpdate, currentTime, config.weatherInterval, needUpdate ? "YES" : "NO");

    return needUpdate;
}

uint8_t TimingManager::getEffectiveDisplayMode() {
    RTCConfigData& config = ConfigManager::getConfig();

    // Temporary mode overrides configured display mode
    if (config.inTemporaryMode) {
        ESP_LOGI(TAG, "getEffectiveDisplayMode: using temporary mode %d", config.temporaryDisplayMode);
        return config.temporaryDisplayMode;
    }

    switch (config.displayMode) {
    case DISPLAY_MODE_TRANSPORT_ONLY:
    case DISPLAY_MODE_WEATHER_ONLY:
        return config.displayMode;
    case DISPLAY_MODE_HALF_AND_HALF:
        if (isTransportActiveTime()) {
            return DISPLAY_MODE_HALF_AND_HALF;
        }
        return DISPLAY_MODE_WEATHER_ONLY;
    default:
        return DISPLAY_MODE_WEATHER_ONLY;
    }
}

int TimingManager::parseTimeString(const String& timeStr) {
    // Parse "HH:MM" format to minutes since midnight
    int colonPos = timeStr.indexOf(':');
    if (colonPos == -1) return 0;
    int hours = timeStr.substring(0, colonPos).toInt();
    int minutes = timeStr.substring(colonPos + 1).toInt();

    return hours * 60 + minutes;
}

bool TimingManager::isTimeInRange(uint32_t currentMinutes, uint32_t startMinutes, uint32_t endMinutes) {
    if (startMinutes <= endMinutes) {
        // Same day range (e.g., 06:00 to 22:00)
        // Use < instead of <= for end to exclude boundary
        return currentMinutes >= startMinutes && currentMinutes < endMinutes;
    } else {
        // Overnight range (e.g., 22:30 to 05:30)
        // currentMinutes >= 22:30 OR currentMinutes < 05:30 (not <=)
        return currentMinutes >= startMinutes || currentMinutes < endMinutes;
    }
}

uint32_t TimingManager::getLastWeatherUpdate() {
    return lastWeatherUpdate;
}

uint32_t TimingManager::getLastTransportUpdate() {
    return lastTransportUpdate;
}

void TimingManager::setLastWeatherUpdate(uint32_t timestamp) {
    lastWeatherUpdate = timestamp;
}

void TimingManager::setLastTransportUpdate(uint32_t timestamp) {
    lastTransportUpdate = timestamp;
}

uint32_t TimingManager::getLastOTACheck() {
    return lastOTACheck;
}

void TimingManager::setLastOTACheck(uint32_t timestamp) {
    lastOTACheck = timestamp;
}

uint32_t TimingManager::calculateNextOTACheckTime(uint32_t currentTimeSeconds) {
    RTCConfigData& config = ConfigManager::getConfig();

    // Check if OTA is enabled
    if (!config.otaEnabled) {
        ESP_LOGD(TAG, "OTA automatic updates are disabled");
        return 0; // Return 0 to indicate OTA is not scheduled
    }

    // Check if we already checked OTA recently (within last 2 minutes to avoid repeated checks)
    if (lastOTACheck > 0 && (currentTimeSeconds - lastOTACheck) < 120) {
        ESP_LOGD(TAG, "OTA check already performed recently (within 2 minutes)");
        return 0; // Skip OTA check
    }

    // Parse configured OTA check time (format: "HH:MM")
    int otaCheckMinutes = parseTimeString(String(config.otaCheckTime));

    // Get current time info
    time_t currentTime = (time_t)currentTimeSeconds;
    tm currentTm;
    localtime_r(&currentTime, &currentTm);
    int currentMinutes = currentTm.tm_hour * 60 + currentTm.tm_min;

    // Calculate next OTA check time
    uint32_t nextOTACheckSeconds = 0;

    if (currentMinutes < otaCheckMinutes) {
        // OTA check time is later today
        int minutesUntilOTA = otaCheckMinutes - currentMinutes;
        nextOTACheckSeconds = currentTimeSeconds + (minutesUntilOTA * 60);
        ESP_LOGD(TAG, "Next OTA check is later today in %d minutes at %02d:%02d",
                 minutesUntilOTA, otaCheckMinutes / 60, otaCheckMinutes % 60);
    } else {
        // OTA check time is tomorrow
        int minutesUntilMidnight = (24 * 60) - currentMinutes;
        int minutesUntilOTA = minutesUntilMidnight + otaCheckMinutes;
        nextOTACheckSeconds = currentTimeSeconds + (minutesUntilOTA * 60);
        ESP_LOGD(TAG, "Next OTA check is tomorrow in %d minutes at %02d:%02d",
                 minutesUntilOTA, otaCheckMinutes / 60, otaCheckMinutes % 60);
    }

    ESP_LOGI(TAG, "Next OTA check scheduled at: %u seconds", nextOTACheckSeconds);
    return nextOTACheckSeconds;
}

