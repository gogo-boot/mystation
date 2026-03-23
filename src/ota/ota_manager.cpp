#include "config/config_manager.h"
#include "util/time_manager.h"
#include "util/timing_manager.h"
#include "ota/ota_manager.h"
#include "ota/ota_update.h"
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

static void otaTask(void* param) {
    check_ota_update();

    // Notify the caller that OTA finished without a reboot (no update / error)
    // Use a local copy of the handle in case of race conditions
    TaskHandle_t caller = s_callerTask;
    s_callerTask = nullptr; // clear before notify to prevent double-notify

    if (caller != nullptr) {
        xTaskNotifyGive(caller);
    }

    // Delete this task
    vTaskDelete(nullptr);
}

static void runOtaInDedicatedTask() {
    s_callerTask = xTaskGetCurrentTaskHandle();

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
        check_ota_update();
        return;
    }

    // Block indefinitely; if OTA succeeds the device reboots and we never wake.
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}

// Set to true when the user explicitly triggers OTA via Button 3 long press.
// Lives in normal RAM — valid only within the current boot, not across deep sleep.
static bool userRequestedUpdate = false;

namespace OTAManager {
    void requestUpdateByUser() {
        ESP_LOGI(TAG, "OTA update requested by user (Button 3 long press)");
        userRequestedUpdate = true;
    }

    bool shouldCheckForUpdate() {
#if OTA_FORCE_UPDATE
        ESP_LOGW(TAG, "OTA_FORCE_UPDATE is enabled – skipping time/config check");
        return true;
#else
        // User explicitly triggered OTA via button — bypass schedule and config flag
        if (userRequestedUpdate) {
            ESP_LOGI(TAG, "User-requested OTA update – bypassing schedule check");
            return true;
        }

        RTCConfigData& config = ConfigManager::getConfig();

        // Check if OTA is enabled
        if (!config.otaEnabled) {
            ESP_LOGD(TAG, "OTA automatic updates are disabled");
            return false;
        }

        // Get current time
        tm timeinfo = {};
        if (!TimeManager::getCurrentLocalTime(timeinfo)) {
            ESP_LOGW(TAG, "Failed to get current time for OTA check");
            return false;
        }

        // Parse configured OTA check time (format: "HH:MM")
        int configHour = 0;
        int configMinute = 0;
        if (sscanf(config.otaCheckTime, "%d:%d", &configHour, &configMinute) != 2) {
            ESP_LOGW(TAG, "Invalid OTA check time format: %s", config.otaCheckTime);
            return false;
        }

        // Get current time
        int currentHour = timeinfo.tm_hour;
        int currentMinute = timeinfo.tm_min;

        // Check if current time is within 1 minute tolerance of configured time
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
#endif // OTA_FORCE_UPDATE
    }

    void checkAndApplyUpdate() {
        if (shouldCheckForUpdate()) {
            ESP_LOGI(TAG, "Starting OTA update check...");

            // Reset user-request flag before running so normal scheduling
            // is not affected if the OTA check completes without a restart
            userRequestedUpdate = false;

            runOtaInDedicatedTask();

            // Mark OTA check timestamp to prevent repeated checks
            time_t now;
            time(&now);
            TimingManager::setLastOTACheck((uint32_t)now);

            // Note: If update is found and installed, device will restart
            // If no update or update fails, execution continues normally
        }
    }
} // namespace OTAManager

