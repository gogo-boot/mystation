#include "build_config.h"
#include "util/factory_reset.h"
#include "config/pins.h"
#include <nvs_flash.h>

bool FactoryReset::checkResetButton() {
    if (HAS_BUTTON) {
        // Check if button is currently pressed
        if (digitalRead(Pins::GPIO_BUTTON_1) == LOW && digitalRead(Pins::GPIO_BUTTON_2) == LOW) {
            Serial.println("🔵 Reset button detected!");
            Serial.println("   Hold button for 5 seconds to factory reset...");

            unsigned long startTime = millis();

            // Monitor button for 3 seconds
            while (millis() - startTime < FACTORY_RESET_HOLD_DURATION_MS) {
                // Check if button was released
                if (digitalRead(Pins::GPIO_BUTTON_1) == HIGH || digitalRead(Pins::GPIO_BUTTON_2) == HIGH) {
                    unsigned long heldDuration = millis() - startTime;
                    Serial.printf("🟢 Button released after %.1f seconds\n", heldDuration / 1000.0);
                    Serial.println("   (Not long enough for factory reset)\n");
                    return false;
                }

                // Show progress every second
                unsigned long elapsed = millis() - startTime;
                static unsigned long lastProgressTime = 0;
                if (elapsed - lastProgressTime >= 1000) {
                    unsigned long secondsRemaining = (FACTORY_RESET_HOLD_DURATION_MS - elapsed) / 1000;
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


void FactoryReset::performReset() {
    Serial.println("\n🔥 ================================");
    Serial.println("🔥 FACTORY RESET INITIATED!");
    Serial.println("🔥 ================================\n");

    Serial.println("🗑️  Erasing NVS (Non-Volatile Storage)...");
    esp_err_t err = nvs_flash_erase();
    if (err == ESP_OK) {
        Serial.println("✅ NVS erased successfully!");

        // Reinitialize NVS
        err = nvs_flash_init();
        if (err == ESP_OK) {
            Serial.println("✅ NVS reinitialized successfully!");
        } else {
            Serial.printf("⚠️  NVS reinitialization failed: %s\n", esp_err_to_name(err));
        }
    } else {
        Serial.printf("❌ NVS erase failed: %s\n", esp_err_to_name(err));
    }

    Serial.println("\n✨ Factory reset complete!");
    Serial.println("   Counter will start from 0 again.");

    // Restart the device
    ESP.restart();
}
