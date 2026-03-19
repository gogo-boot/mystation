#include "util/system_init.h"
#include "util/factory_reset.h"
#include "util/application_reset.h"
#include "util/button_monitor.h"
#include "config/config_manager.h"
#include <esp_log.h>
#include <nvs_flash.h>
#include <time.h>

#include "display/display_manager.h"
#include "global_instances.h"

static const char* TAG = "SYSTEM_INIT";

// How long a button must be held to trigger any long-press action (ms)
static constexpr unsigned long BUTTON_HOLD_DURATION_MS = 3000;

// Flag: true if a long press was handled this boot — used to skip short-press wakeup handling
static bool longPressHandled = false;

namespace SystemInit {
    void initSerialConnector() {
        esp_log_level_set("*", ESP_LOG_DEBUG);
        Serial.begin(115200);
        delay(1000);
    }

    bool wasLongPressHandled() {
        return longPressHandled;
    }

    void handleButtonLongPressActions() {
        uint8_t held = ButtonMonitor::detectLongPress(BUTTON_HOLD_DURATION_MS);

        if (held == BUTTON_1_AND_2) {
            // Button 1 + Button 2 held → Factory Reset
            Serial.println("🔥 Factory reset triggered by Button 1 + Button 2");
            nvs_flash_init();
            FactoryReset::performReset();
        } else if (held == BUTTON_1_HELD) {
            // Button 1 only → Application Reset
            Serial.println("🔄 Application reset triggered by Button 1");
            nvs_flash_init();
            AppicationReset::performReset();
        } else if (held == BUTTON_2_HELD) {
            // Button 2 only → Application Info temporary display mode
            // NOTE: Do NOT overwrite config.displayMode — that is the user's persistent
            // configured mode. Only set the temporary overlay so that after 2 minutes
            // the device correctly returns to the configured mode.
            Serial.println("ℹ️  Application info mode triggered by Button 2");
            RTCConfigData& config = ConfigManager::getConfig();
            time_t now;
            time(&now);
            config.inTemporaryMode = true;
            config.temporaryDisplayMode = DISPLAY_MODE_APPLICATION_INFO;
            config.temporaryModeActivationTime = (uint32_t)now;
            longPressHandled = true;
            ESP_LOGI(TAG, "Temporary APPLICATION_INFO mode set, activation time: %u",
                     config.temporaryModeActivationTime);
        } else if (held == BUTTON_3_HELD) {
            // Button 3 only → Reserved for future use
            Serial.println("🔵 Button 3 long press detected (reserved)");
        }
    }

    void initDisplay() {
        // Info : initial Parameter can be used to preserve screen content for partial updates
        display.init(DisplayConstants::SERIAL_BAUD_RATE, true,
                     DisplayConstants::RESET_DURATION_MS, false);
        // Landscape orientation
        display.setRotation(0);
    }

    void initFont() {
        // Initialize U8g2 for UTF-8 font support (German umlauts)
        u8g2.begin(display);
        u8g2.setFontMode(1); // Use u8g2 transparent mode
        u8g2.setFontDirection(0); // Left to right
        u8g2.setForegroundColor(GxEPD_BLACK);
        u8g2.setBackgroundColor(GxEPD_WHITE);
    }

    void loadNvsConfig() {
        // Restore RTCConfigData from NVS
        ConfigManager& configMgr = ConfigManager::getInstance();
        configMgr.loadFromNVS(false);
        ESP_LOGI(TAG, "System initialization complete");
    }
} // namespace SystemInit
