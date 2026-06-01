#include "util/boot_flow_manager.h"
#include "util/device_mode_manager.h"
#include "util/wifi_manager.h"
#include "config/config_manager.h"

#include "util/timing_manager.h"
#include "global_instances.h"

static const char* TAG = "BOOT_FLOW";

namespace BootFlowManager {

    static void runOperationalMode(uint8_t displayMode) {
        switch (displayMode) {
        case DISPLAY_MODE_HALF_AND_HALF:
            ESP_LOGI(TAG, "Starting Weather + Departure half-and-half mode");
            DeviceModeManager::showWeatherDeparture();
            break;

        case DISPLAY_MODE_WEATHER_ONLY:
            ESP_LOGI(TAG, "Starting Weather-only full screen mode");
            DeviceModeManager::updateWeatherFull();
            break;

        case DISPLAY_MODE_TRANSPORT_ONLY:
            ESP_LOGI(TAG, "Starting Departure-only full screen mode");
            DeviceModeManager::updateDepartureFull();
            break;

        case DISPLAY_MODE_APPLICATION_INFO:
            ESP_LOGI(TAG, "Starting Application Info mode");
            DeviceModeManager::showApplicationInfo();
            break;

        default:
            ESP_LOGW(TAG, "Unknown display mode %d, defaulting to half-and-half", displayMode);
            DeviceModeManager::showWeatherDeparture();
            break;
        }
    }

    void handlePhaseWifiSetup() {
        ESP_LOGI(TAG, "==========================================");
        ESP_LOGI(TAG, "=== PHASE 1: WiFi Setup ===");
        ESP_LOGI(TAG, "==========================================");

        DeviceModeManager::showPhaseInstructions(PHASE_WIFI_SETUP);
        ConfigManager::setDefaults();

        WiFiManager wm;
        MyWiFiManager::setupWiFiAccessPointAndRestart(wm);
    }

    void handlePhaseAppSetup() {
        ESP_LOGI(TAG, "Phase 2: Application Setup Required");

        MyWiFiManager::reconnectWiFi();

        if (MyWiFiManager::isConnected() && MyWiFiManager::hasInternetAccess()) {
            DeviceModeManager::runConfigurationMode();
            DeviceModeManager::showPhaseInstructions(PHASE_APP_SETUP);
            DeviceModeManager::startWebServer();
        } else {
            ESP_LOGE(TAG, "WiFi validation failed - reverting to Phase 1");
            DeviceModeManager::logWifiError();
            handlePhaseWifiSetup();
        }
    }

    void handlePhaseComplete() {
        ESP_LOGI(TAG, "Phase 3: All configured - Running operational mode");

        RTCConfigData& config = ConfigManager::getConfig();

        // Sanitize APPLICATION_INFO if it leaked into persistent config
        if (config.displayMode == DISPLAY_MODE_APPLICATION_INFO) {
            ESP_LOGW(TAG, "config.displayMode is APPLICATION_INFO — invalid as persistent mode, resetting to WEATHER_ONLY");
            config.displayMode = DISPLAY_MODE_WEATHER_ONLY;
            ConfigManager::getInstance().saveToNVS();
        }

        uint8_t displayMode = TimingManager::getEffectiveDisplayMode();

        ESP_LOGI(TAG, "Display mode determined: %d (temp=%d, configured=%d)",
                 displayMode, config.inTemporaryMode, config.displayMode);

        runOperationalMode(displayMode);
    }

} // namespace BootFlowManager
