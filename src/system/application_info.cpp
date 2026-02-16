#include "system/application_info.h"
#include "config/config_manager.h"

bool AppicationInfo::checkButtonHold(int buttonPin) {
#ifdef BOARD_ESP32_S3
    // Check if button is currently pressed
    if (digitalRead(buttonPin) == LOW) {
        Serial.println("🔵 Reset button detected!");
        Serial.println("   Hold button for 5 seconds to display application info...");

        unsigned long startTime = millis();

        // Monitor button for 3 seconds
        while (millis() - startTime < APPLICATION_INFO_HOLD_DURATION_MS) {
            // Check if button was released
            if (digitalRead(buttonPin) == HIGH) {
                unsigned long heldDuration = millis() - startTime;
                Serial.printf("🟢 Button released after %.1f seconds\n", heldDuration / 1000.0);
                Serial.println("   (Not long enough for factory reset)\n");
                return false;
            }

            // Show progress every second
            unsigned long elapsed = millis() - startTime;
            static unsigned long lastProgressTime = 0;
            if (elapsed - lastProgressTime >= 1000) {
                unsigned long secondsRemaining = (APPLICATION_INFO_HOLD_DURATION_MS - elapsed) / 1000;
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
#else
    // Factory reset button not available on other boards
    return false;
#endif
}


void AppicationInfo::showApplicationInfo() {
    Serial.println("\n🔥 ================================");
    Serial.println("🔥 APPLICATION INFO DISPLAY INITIATED!");
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
