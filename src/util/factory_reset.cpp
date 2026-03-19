#include "build_config.h"
#include "util/factory_reset.h"
#include <nvs_flash.h>


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
