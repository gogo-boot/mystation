#include "build_config.h"
#include "util/application_reset.h"
#include "config/pins.h"
#include "config/config_manager.h"

bool AppicationReset::checkResetButton() {
    if (HAS_BUTTON) {
        // Check if button is currently pressed
        if (digitalRead(Pins::GPIO_BUTTON_1) == LOW) {
            Serial.println("🔵 Reset button detected!");
            Serial.println("   Hold button for 5 seconds to factory reset...");

            unsigned long startTime = millis();

            // Monitor button for 3 seconds
            while (millis() - startTime < APPLICATION_RESET_HOLD_DURATION_MS) {
                // Check if button was released
                if (digitalRead(Pins::GPIO_BUTTON_1) == HIGH) {
                    unsigned long heldDuration = millis() - startTime;
                    Serial.printf("🟢 Button released after %.1f seconds\n", heldDuration / 1000.0);
                    Serial.println("   (Not long enough for factory reset)\n");
                    return false;
                }

                // Show progress every second
                unsigned long elapsed = millis() - startTime;
                static unsigned long lastProgressTime = 0;
                if (elapsed - lastProgressTime >= 1000) {
                    unsigned long secondsRemaining = (APPLICATION_RESET_HOLD_DURATION_MS - elapsed) / 1000;
                    if (secondsRemaining > 0) {
                        Serial.printf("⏱️  Holding... %lu seconds remaining\n", secondsRemaining);
                    }
                    lastProgressTime = elapsed;
                }

                delay(50); // Small delay for debouncing
            }

            // Button was held for full 5 seconds
            Serial.println("✅ Button held for 5 seconds!");
            return true;
        }

        return false;
    } else {
        // Factory reset button not available on other boards
        return false;
    }
}


void AppicationReset::performReset() {
    Serial.println("\n🔥 ================================");
    Serial.println("🔥 APPLICATION RESET INITIATED!");
    Serial.println("🔥 ================================\n");

    // Get ConfigManager instance
    ConfigManager& configMgr = ConfigManager::getInstance();

    RTCConfigData& config = ConfigManager::getConfig();

    config.latitude = 0.0;
    config.longitude = 0.0;
    // Reset transport data
    memset(config.selectedStopId, 0, sizeof(config.selectedStopId));

    configMgr.saveToNVS();

    Serial.println("\n✨ Application reset complete!");
    Serial.println("   Counter will start from 0 again.");

    // Restart the device
    ESP.restart();
}
