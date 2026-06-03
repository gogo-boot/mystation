#include "config/config_manager.h"
#include "util/time_manager.h"
#include "util/timing_manager.h"
#include "ota/ota_manager.h"
#include "ota/ota_update.h"
#include "display/display_manager.h"
#include "build_config.h"
#include <ctime>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char* TAG = "OTA_MANAGER";

// The OTA work (TLS handshake + mbedTLS + flash write) is very stack-heavy.
// Running it on the main Arduino task (default ~8 KB stack) causes a stack
// overflow that manifests as ESP_ERR_INVALID_ARG from esp_https_ota_perform()
// and a subsequent LoadProhibited/StoreProhibited panic.
// Solution: spin up a dedicated task with a large enough stack (16 KB), run
// OTA there, and block the caller via a task notification until it finishes.
// If OTA installs a new image it will call esp_restart() from inside the task,
// so the caller will never be unblocked in that case.

static TaskHandle_t s_callerTask = nullptr;
static OTAResult s_otaResult = OTA_UPDATE_FAILED;

static void otaTask(void* param) {
    s_otaResult = check_ota_update();

    // Notify the caller that OTA finished without a reboot (no update / error)
    TaskHandle_t caller = s_callerTask;
    s_callerTask = nullptr;

    if (caller != nullptr) {
        xTaskNotifyGive(caller);
    }

    vTaskDelete(nullptr);
}

static OTAResult runOtaInDedicatedTask() {
    s_callerTask = xTaskGetCurrentTaskHandle();
    s_otaResult = OTA_UPDATE_FAILED;

    BaseType_t created = xTaskCreate(
        otaTask,
        "ota_task",
        16384, // 16 KB – enough for TLS handshake + mbedTLS + OTA flash write
        nullptr,
        5, // slightly above normal so it isn't starved
        nullptr
    );

    if (created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create OTA task – running inline (may stack overflow)");
        return check_ota_update();
    }

    // Block indefinitely; if OTA succeeds the device reboots and we never wake.
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    return s_otaResult;
}

// Set to true when the user explicitly triggers OTA via Button 3 long press.
static bool userRequestedUpdate = false;

namespace OTAManager {
    void requestUpdateByUser() {
        ESP_LOGI(TAG, "OTA update requested by user (Button 3 long press)");
        userRequestedUpdate = true;
    }

    bool shouldCheckForUpdate() {
        if (userRequestedUpdate) {
            ESP_LOGI(TAG, "User-requested OTA update – bypassing schedule check");
            return true;
        }

        RTCConfigData& config = ConfigManager::getConfig();

        if (!config.otaEnabled) {
            ESP_LOGD(TAG, "OTA automatic updates are disabled");
            return false;
        }

        tm timeinfo = {};
        if (!TimeManager::getCurrentLocalTime(timeinfo)) {
            ESP_LOGW(TAG, "Failed to get current time for OTA check");
            return false;
        }

        int configHour = 0;
        int configMinute = 0;
        if (sscanf(config.otaCheckTime, "%d:%d", &configHour, &configMinute) != 2) {
            ESP_LOGW(TAG, "Invalid OTA check time format: %s", config.otaCheckTime);
            return false;
        }

        int currentHour = timeinfo.tm_hour;
        int currentMinute = timeinfo.tm_min;

        bool isTimeMatch = (currentHour == configHour &&
            abs(currentMinute - configMinute) <= 1);

        if (isTimeMatch) {
            ESP_LOGI(TAG, "OTA update time matched! Configured: %s, Current: %02d:%02d",
                     config.otaCheckTime, currentHour, currentMinute);
            return true;
        }

        ESP_LOGD(TAG, "OTA update time not matched. Configured: %s, Current: %02d:%02d",
                 config.otaCheckTime, currentHour, currentMinute);
        return false;
    }

    bool checkAndApplyUpdate() {
        if (!shouldCheckForUpdate()) {
            return false;
        }

        ESP_LOGI(TAG, "Starting OTA update check...");
        bool wasUserRequest = userRequestedUpdate;
        userRequestedUpdate = false;

        OTAResult result = runOtaInDedicatedTask();

        // Mark OTA check timestamp
        time_t now;
        time(&now);
        TimingManager::setLastOTACheck((uint32_t)now);

        // Show display feedback only for user-triggered requests
        if (wasUserRequest && result == OTA_UP_TO_DATE) {
            DisplayManager::displayOTAUpToDate(FIRMWARE_VERSION);

            // Set temporary mode so device sleeps ~120s then returns to configured display
            RTCConfigData& config = ConfigManager::getConfig();
            config.inTemporaryMode = true;
            config.temporaryDisplayMode = config.displayMode;
            time_t t;
            time(&t);
            config.temporaryModeActivationTime = (uint32_t)t;

            return true; // Signal caller to skip normal rendering
        }

        return false;
    }
} // namespace OTAManager

