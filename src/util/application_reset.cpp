#include "build_config.h"
#include "util/application_reset.h"
#include "config/config_manager.h"


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
