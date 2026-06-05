#include "util/wifi_manager.h"
#include "config/config_manager.h"
#include "util/util.h"

static const char* TAG = "WIFI_MGR";

// RTC variables to persist WiFi state across deep sleep


void MyWiFiManager::reconnectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGD(TAG, "WiFi already connected: %s", WiFi.localIP().toString().c_str());
        return; // Already connected
    }

    WiFi.begin();

    int attempts = 0;
    const int max_attempts = FULL_CONNECT_TIMEOUT_MS / 500;

    while (WiFi.status() != WL_CONNECTED && attempts < max_attempts) {
        delay(500);
        ESP_LOGD(TAG, "Full scan connecting... attempt %d/%d", attempts + 1, max_attempts);
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGI(TAG, "WiFi reconnected successfully via full scan!");
        ESP_LOGI(TAG, "IP address: %s", WiFi.localIP().toString().c_str());
        ESP_LOGI(TAG, "Connected to SSID: %s", WiFi.SSID().c_str());
    } else {
        ESP_LOGW(TAG, "Failed to reconnect to WiFi with saved credentials");
    }
}

void MyWiFiManager::setupWiFiAccessPointAndRestart(WiFiManager& wm) {
    ESP_LOGI(TAG, "Starting WiFi AP for Phase 1 configuration...");

    // Configure WiFiManager
    const char* menu[] = {"wifi"};
    wm.setMenu(menu, 1);
    wm.setConnectTimeout(20); // 20 seconds per connection attempt
    wm.setConnectRetries(3);
    wm.setMinimumSignalQuality(8); // Lower signal quality requirement
    wm.setAPStaticIPConfig(IPAddress(10, 0, 1, 1), IPAddress(10, 0, 1, 1), IPAddress(255, 255, 255, 0));
    wm.setTitle("MyStation WiFi Setup");
    wm.setCountry("DE");

    // Redirect root page to /wifi so captive portal shows SSID list immediately
    wm.setWebServerCallback([&wm]() {
        wm.server->on("/", HTTP_GET, [&wm]() {
            wm.server->sendHeader("Location", "/wifi", true);
            wm.server->send(302, "text/plain", "");
        });
    });

    // Start AP and wait for WiFi configuration
    String apName = Util::getUniqueSSID("MyStation");
    ESP_LOGI(TAG, "Starting AP with SSID: %s", apName.c_str());

    // Start AP
    // It holds further process until user configures WiFi
    wm.autoConnect(apName.c_str());

    // Check internet connectivity
    if (hasInternetAccess()) {
        ESP_LOGW(TAG, "has internet access");
        // Update configuration with network info
        ConfigManager::setNetwork(WiFi.SSID(), WiFi.localIP().toString());

        // Save WiFi credentials to NVS
        ConfigManager& configMgr = ConfigManager::getInstance();
        ESP_LOGI(TAG, "Saving WiFi credentials to NVS:");
        ESP_LOGI(TAG, "  SSID: %s", WiFi.SSID().c_str());
        ESP_LOGI(TAG, "  IP: %s", WiFi.localIP().toString().c_str());
        configMgr.saveToNVS();
        ESP.restart();
    }
}

bool MyWiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String MyWiFiManager::getLocalIP() {
    return WiFi.localIP().toString();
}

bool MyWiFiManager::hasInternetAccess() {
    ESP_LOGI(TAG, "Checking internet connectivity via DNS...");

    IPAddress dnsResult;
    if (WiFi.hostByName("www.google.com", dnsResult) == 1) {
        ESP_LOGI(TAG, "DNS lookup successful: %s - internet accessible", dnsResult.toString().c_str());
        return true;
    }

    ESP_LOGW(TAG, "DNS lookup failed - no internet access");
    return false;
}
