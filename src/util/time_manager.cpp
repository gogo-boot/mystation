#include "util/time_manager.h"
#include <time.h>
#include <esp_sntp.h>

static const char* TAG = "TIME_MGR";

String TimeManager::getGermanDateTimeString() {
    tm timeinfo;
    if (!getCurrentLocalTime(timeinfo)) {
        ESP_LOGW(TAG, "Failed to get local time for German date/time string");
        return String("--:-- --.--.---- -------");
    }

    static const char* dayNames[] = {
        "Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag"
    };
    char buf[40];
    int wday = timeinfo.tm_wday;
    if (wday < 0 || wday > 6) wday = 0;

    snprintf(buf, sizeof(buf), "%02d:%02d %02d.%02d.%04d %s",
             timeinfo.tm_hour, timeinfo.tm_min,
             timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900,
             dayNames[wday]);
    return String(buf);
}

void TimeManager::printCurrentTime() {
    time_t now = time(nullptr);
    tm* timeinfo = localtime(&now);
    ESP_LOGI(TAG, "Current time: %04d-%02d-%02d %02d:%02d:%02d",
             timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday,
             timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

bool TimeManager::isTimeSet() {
    time_t now = time(nullptr);
    return now > 8 * 3600 * 2; // Check if year > 1971
}

bool TimeManager::getCurrentLocalTime(tm& timeinfo) {
    if (!isTimeSet()) {
        ESP_LOGW(TAG, "Time not set, cannot get current local time");
        return false;
    }

    // Always ensure timezone is set before getting local time
    // This is necessary because TZ environment may not persist across deep sleep
    // German timezone: CET-1CEST,M3.5.0,M10.5.0/3 means:
    // - CET (Central European Time) is UTC+1
    // - CEST (Central European Summer Time) is UTC+2
    // M3.5.0 = DST starts on month 3 (March), week 5 (last), day 0 (Sunday) at 02:00
    // M10.5.0/3 = DST ends on month 10 (October), week 5 (last), day 0 (Sunday) at 03:00
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
    tzset();

    time_t now = time(nullptr);
    localtime_r(&now, &timeinfo);


    // Debug: Log current time being returned
    ESP_LOGD(TAG, "getCurrentLocalTime returning: %04d-%02d-%02d %02d:%02d:%02d",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    return true;
}

// ===== ENHANCED TIME MANAGEMENT FOR DEEP SLEEP OPTIMIZATION =====

// RTC variables to persist across deep sleep
RTC_DATA_ATTR static unsigned long lastNtpSyncTime = 0;
RTC_DATA_ATTR static bool hasValidSyncTime = false;

bool TimeManager::needsPeriodicSync() {
    // If we've never synced or don't have valid sync time, we need to sync
    if (!hasValidSyncTime || lastNtpSyncTime == 0) {
        ESP_LOGI(TAG, "First boot or no previous sync time - NTP sync needed");
        return true;
    }

    // Check if enough time has passed since last sync
    unsigned long currentTime = millis();
    unsigned long timeSinceSync = currentTime - lastNtpSyncTime;

    // Handle millis() overflow (happens every ~49 days)
    if (currentTime < lastNtpSyncTime) {
        ESP_LOGI(TAG, "millis() overflow detected - forcing NTP sync");
        return true;
    }

    bool needsSync = timeSinceSync > SYNC_INTERVAL_MS;

    if (needsSync) {
        ESP_LOGI(TAG, "Time since last sync: %lu ms (limit: %lu ms) - NTP sync needed",
                 timeSinceSync, SYNC_INTERVAL_MS);
    } else {
        ESP_LOGD(TAG, "Time since last sync: %lu ms - using RTC time", timeSinceSync);
    }

    return needsSync;
}

void TimeManager::markLastSyncTime() {
    lastNtpSyncTime = millis();
    hasValidSyncTime = true;
    ESP_LOGI(TAG, "Marked NTP sync time: %lu ms", lastNtpSyncTime);
}

unsigned long TimeManager::getTimeSinceLastSync() {
    if (!hasValidSyncTime || lastNtpSyncTime == 0) {
        return ULONG_MAX; // Indicate never synced
    }

    unsigned long currentTime = millis();
    if (currentTime < lastNtpSyncTime) {
        return ULONG_MAX; // millis() overflow
    }

    return currentTime - lastNtpSyncTime;
}

bool TimeManager::setupNTPTimeWithRetry(int maxRetries) {
    ESP_LOGI(TAG, "Setting up NTP time with %d retries", maxRetries);

    for (int attempt = 1; attempt <= maxRetries; attempt++) {
        ESP_LOGI(TAG, "NTP sync attempt %d/%d", attempt, maxRetries);

        // For ESP32, use configTzTime instead of configTime + setenv
        // German timezone: UTC+1 (CET) in winter, UTC+2 (CEST) in summer
        configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov");

        // Wait for sync with shorter timeout per attempt
        time_t now = time(nullptr);
        int retry = 0;
        const int retry_count = 20; // Reduced from 30 for faster retries

        while (now < 8 * 3600 * 2 && retry < retry_count) {
            // year < 1971
            delay(500);
            now = time(nullptr);
            retry++;
        }

        if (now >= 8 * 3600 * 2) {
            // Success - time is set
            tm timeinfo;
            localtime_r(&now, &timeinfo);
            ESP_LOGI(TAG, "NTP sync successful on attempt %d - German time: %04d-%02d-%02d %02d:%02d:%02d",
                     attempt, timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

            markLastSyncTime(); // Mark successful sync
            return true;
        }

        ESP_LOGW(TAG, "NTP sync attempt %d failed after %d retries", attempt, retry_count);

        // Wait before next attempt (except for last attempt)
        if (attempt < maxRetries) {
            ESP_LOGI(TAG, "Waiting 2 seconds before next attempt...");
            delay(2000);
        }
    }

    ESP_LOGE(TAG, "All NTP sync attempts failed after %d tries", maxRetries);
    return false;
}

String TimeManager::formatDurationInHours(unsigned long milliseconds) {
    if (milliseconds == ULONG_MAX) {
        return "never";
    }
    float hours = milliseconds / (1000.0 * 60.0 * 60.0);
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f hours", hours);
    return String(buf);
}

