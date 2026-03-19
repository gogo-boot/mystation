#include "util/system_init.h"
#include "util/factory_reset.h"
#include "util/application_reset.h"
#include "util/button_monitor.h"
#include "util/button_manager.h"
#include "config/config_manager.h"
#include <esp_log.h>
#include <nvs_flash.h>

#include "display/display_manager.h"
#include "global_instances.h"

static const char* TAG = "SYSTEM_INIT";

// How long a button must be held to trigger any long-press action (ms)
static constexpr unsigned long BUTTON_HOLD_DURATION_MS = 3000;

namespace SystemInit {
    void initSerialConnector() {
        esp_log_level_set("*", ESP_LOG_DEBUG);
        Serial.begin(115200);
        delay(1000);
    }

    void handleButtonLongPressActions() {
        uint8_t held = ButtonMonitor::detectLongPress(BUTTON_HOLD_DURATION_MS);

        if (held == BUTTON_1_AND_2) {
            // Button 1 + Button 2 held → Factory Reset (restarts device, never returns)
            Serial.println("🔥 Factory reset triggered by Button 1 + Button 2");
            nvs_flash_init();
            FactoryReset::performReset();
        } else if (held == BUTTON_1_HELD) {
            // Button 1 only → Application Reset (restarts device, never returns)
            Serial.println("🔄 Application reset triggered by Button 1");
            nvs_flash_init();
            AppicationReset::performReset();
        } else if (held == BUTTON_2_HELD) {
            // Button 2 only → Application Info mode.
            // Inject as a synthetic button mode so handleWakeupMode() treats it
            // identically to a short-press button wakeup: same temp-mode logic,
            // same 2-minute timer, same expiry clearing.
            Serial.println("ℹ️  Application info mode triggered by Button 2 long press");
            ButtonManager::setSyntheticButtonMode(DISPLAY_MODE_APPLICATION_INFO);
        } else if (held == BUTTON_3_HELD) {
            // Button 3 only → Reserved for future use
            Serial.println("🔵 Button 3 long press detected (reserved)");
        }
    }

    void initDisplay() {
        display.init(DisplayConstants::SERIAL_BAUD_RATE, true,
                     DisplayConstants::RESET_DURATION_MS, false);
        display.setRotation(0);
    }

    void initFont() {
        u8g2.begin(display);
        u8g2.setFontMode(1);
        u8g2.setFontDirection(0);
        u8g2.setForegroundColor(GxEPD_BLACK);
        u8g2.setBackgroundColor(GxEPD_WHITE);
    }

    void loadNvsConfig() {
        ConfigManager& configMgr = ConfigManager::getInstance();
        configMgr.loadFromNVS(false);
        ESP_LOGI(TAG, "System initialization complete");
    }
} // namespace SystemInit
