#include "config/config_page.h"
#include "config/config_page_html.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <StreamUtils.h>
#include "config/config_manager.h"
#include "config/config_page_data.h"
#include "api/google_api.h"
#include "api/dwd_weather_api.h"
#include "api/rmv_api.h"
#include "util/util.h"
#include "sec/aes_crypto.h"
#include "util/sleep_utils.h"
#include "global_instances.h"

static const char* TAG = "CONFIG";

// Pre-rendered config page HTML (built once at server start)
static String g_renderedPage;

static String renderConfigPageHtml() {
    ConfigPageData& pageData = ConfigPageData::getInstance();
    RTCConfigData& config = ConfigManager::getConfig();

    // Build filters JS array
    std::vector<String> activeFilters = ConfigManager::getActiveFilters();
    String filtersJs = "[";
    for (size_t i = 0; i < activeFilters.size(); i++) {
        if (i > 0) filtersJs += ",";
        filtersJs += "\"" + activeFilters[i] + "\"";
    }
    filtersJs += "]";

    // Build stops HTML (may be empty if lazy-loaded)
    String stopsHtml;
    if (pageData.getStopCount() > 0) {
        stopsHtml = "<option value=''>Bitte wählen...</option>";
        for (size_t i = 0; i < pageData.getStopCount(); ++i) {
            String encodedId = Util::urlEncode(pageData.getStopId(i));
            stopsHtml += "<option value='" + encodedId + "'>" + pageData.getStopName(i) + "   (" +
                pageData.getStopDistance(i) + "m)</option>";
        }
    } else {
        stopsHtml = "<option value=''>Bitte wählen...</option>";
    }

    // Build full page with template replacement
    const char* html = CONFIG_PAGE_HTML;
    const size_t len = strlen(html);
    size_t pos = 0;

    String page;
    page.reserve(len + 1024);

    while (pos < len) {
        if (html[pos] == '{' && pos + 1 < len && html[pos + 1] == '{') {
            pos += 2;
            const char* varStart = html + pos;
            while (pos < len && html[pos] != '}') pos++;
            String varName(varStart, html + pos - varStart);
            if (pos < len) pos += 2;

            if (varName == "DISPLAY_MODE") { page += String(config.displayMode); }
            else if (varName == "WEATHER_INTERVAL") { page += String(config.weatherInterval); }
            else if (varName == "WEATHER_MODEL") { page += config.weatherModel; }
            else if (varName == "TRANSPORT_INTERVAL") { page += String(config.transportInterval); }
            else if (varName == "TRANSPORT_ACTIVE_START") { page += config.transportActiveStart; }
            else if (varName == "TRANSPORT_ACTIVE_END") { page += config.transportActiveEnd; }
            else if (varName == "WALKING_TIME") { page += String(config.walkingTime); }
            else if (varName == "SLEEP_START") { page += config.sleepStart; }
            else if (varName == "SLEEP_END") { page += config.sleepEnd; }
            else if (varName == "WEEKEND_MODE") { page += config.weekendMode ? "checked" : ""; }
            else if (varName == "WEEKEND_TRANSPORT_START") { page += config.weekendTransportStart; }
            else if (varName == "WEEKEND_TRANSPORT_END") { page += config.weekendTransportEnd; }
            else if (varName == "WEEKEND_SLEEP_START") { page += config.weekendSleepStart; }
            else if (varName == "WEEKEND_SLEEP_END") { page += config.weekendSleepEnd; }
            else if (varName == "OTA_ENABLED") { page += config.otaEnabled ? "true" : "false"; }
            else if (varName == "OTA_CHECK_TIME") { page += config.otaCheckTime; }
            else if (varName == "SAVED_FILTERS") { page += filtersJs; }
            else if (varName == "LAT") { page += String(pageData.getLatitude(), 6); }
            else if (varName == "LON") { page += String(pageData.getLongitude(), 6); }
            else if (varName == "CITY") { page += pageData.getCityName(); }
            else if (varName == "STOPS") { page += stopsHtml; }
        } else {
            page += html[pos];
            pos++;
        }
    }

    return page;
}

void handleConfigPage(WebServer& server) {
    server.send(200, "text/html; charset=utf-8", g_renderedPage);
}

// Save configuration handler (POST /save_config)
void handleSaveConfig(WebServer& server) {
    if (server.method() != HTTP_POST) {
        server.send(405, "text/plain", "Method Not Allowed");
        return;
    }

    ConfigManager& configMgr = ConfigManager::getInstance();

    // Parse JSON body
    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, server.arg("plain"));
    if (err) {
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }

    // Decode stopId if present
    if (doc.containsKey("stopId")) {
        String stopId = doc["stopId"].as<String>();
        doc["stopId"] = Util::urlDecode(stopId);
    }

    ESP_LOGI(TAG, "Saving configuration from web interface");

    // Update config struct from JSON
    configMgr.updateFromJson(doc);

    // Save to NVS (and RTC memory automatically)
    if (!configMgr.saveToNVS()) {
        server.send(500, "text/plain", "Failed to save config to NVS");
        return;
    }

    // Switch to station mode only
#ifdef ESP32
    WiFi.mode(WIFI_STA);
    WiFi.softAPdisconnect(true);
#endif

    server.send(200, "application/json", "{\"status\":\"ok\"}");
    enterDeepSleep(1);
}

// AJAX handler to search cities by postal code (GET /api/city?q=...)
void handleCityAutocomplete(WebServer& server) {
    String query = server.hasArg("q") ? server.arg("q") : "";

    // Validate postal code: must be 5 digits
    if (query.length() < 5) {
        server.send(200, "application/json", "[]");
        return;
    }

    // Validate that it's numeric
    for (size_t i = 0; i < query.length(); i++) {
        if (!isdigit(query.charAt(i))) {
            server.send(200, "application/json", "[]");
            return;
        }
    }

    ESP_LOGI(TAG, "Postal code search query: %s", query.c_str());

    HTTPClient http;
    String url = "https://nominatim.openstreetmap.org/search?postalcode=" + Util::urlEncode(query) +
        "&format=json&limit=5&addressdetails=1&countrycodes=de";

    http.begin(url);
    http.addHeader("User-Agent", "ESP32-MyStation/1.0");

    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        ESP_LOGD(TAG, "Nominatim response: %s", payload.c_str());

        // Parse the response and extract city, lat, lon
        DynamicJsonDocument docIn(4096);
        DeserializationError error = deserializeJson(docIn, payload);

        if (!error) {
            DynamicJsonDocument docOut(2048);
            JsonArray outArray = docOut.to<JsonArray>();

            JsonArray results = docIn.as<JsonArray>();
            for (JsonObject result : results) {
                JsonObject city = outArray.createNestedObject();

                // Get display name or city name
                String displayName = result["display_name"].as<String>();
                String lat = result["lat"].as<String>();
                String lon = result["lon"].as<String>();
                // Try to extract city name from address
                String cityName = displayName;
                if (result.containsKey("address")) {
                    JsonObject addr = result["address"];
                    if (addr.containsKey("city")) {
                        cityName = addr["city"].as<String>();
                    } else if (addr.containsKey("town")) {
                        cityName = addr["town"].as<String>();
                    } else if (addr.containsKey("village")) {
                        cityName = addr["village"].as<String>();
                    }
                }

                city["name"] = cityName;
                city["display"] = displayName;
                city["lat"] = lat;
                city["lon"] = lon;
            }

            String out;
            serializeJson(docOut, out);
            http.end();
            server.send(200, "application/json", out);
            return;
        } else {
            ESP_LOGE(TAG, "Failed to parse Nominatim JSON: %s", error.c_str());
        }
    } else {
        ESP_LOGE(TAG, "Nominatim API failed: %s", http.errorToString(httpCode).c_str());
    }

    http.end();
    server.send(200, "application/json", "[]");
}

// AJAX handler to resolve station/stop name (GET /api/stop?q=...)
void handleStopAutocomplete(WebServer& server) {
    String query = server.hasArg("q") ? server.arg("q") : "";

    if (query.length() < 3) {
        server.send(200, "application/json", "[]");
        return;
    }

    ESP_LOGI(TAG, "Station search query: %s", query.c_str());

    // Use static utility method for secure API key decryption
    std::string decrypted = AESCrypto::getRMVAPIKey();

    HTTPClient http;
    String url = "https://www.rmv.de/hapi/location.name?accessId=" + String(decrypted.c_str()) +
        "&input=" + Util::urlEncode(query) +
        "&format=json&maxNo=5";

    String urlForLog = url;
    int keyPos = urlForLog.indexOf("accessId=");
    if (keyPos != -1) {
        int keyEnd = urlForLog.indexOf('&', keyPos);
        if (keyEnd == -1) keyEnd = urlForLog.length();
        urlForLog.replace(urlForLog.substring(keyPos, keyEnd), "accessId=***");
    }

    ESP_LOGI(TAG, "Requesting RMV location search: %s", urlForLog.c_str());

    http.begin(url);

    // Collect Transfer-Encoding header to handle chunked responses
    const char* keys[] = {"Transfer-Encoding"};
    http.collectHeaders(keys, 1);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        ESP_LOGE(TAG, "RMV API failed: %s", http.errorToString(httpCode).c_str());
        http.end();
        server.send(200, "application/json", "[]");
        return;
    }

    // Create JSON filter to only parse "id" and "name" fields
    StaticJsonDocument<128> stationFilter;
    stationFilter["stopLocationOrCoordLocation"][0]["StopLocation"]["id"] = true;
    stationFilter["stopLocationOrCoordLocation"][0]["StopLocation"]["name"] = true;

    // Create the raw and decoded stream
    Stream& rawStream = http.getStream();
    ChunkDecodingStream decodedStream(http.getStream());

    // Choose the stream based on the Transfer-Encoding header
    Stream& response = http.header("Transfer-Encoding") == "chunked" ? decodedStream : rawStream;

    // Use smaller JSON document since we're only extracting id and name
    DynamicJsonDocument docIn(2048); // Reduced from 4096
    DeserializationOption::NestingLimit nestingLimit(15);

    // Parse with filter to minimize memory usage
    DeserializationError error = deserializeJson(docIn, response,
                                                 DeserializationOption::Filter(stationFilter),
                                                 nestingLimit);

    if (error) {
        ESP_LOGE(TAG, "Failed to parse RMV JSON: %s", error.c_str());
        if (error == DeserializationError::NoMemory) {
            ESP_LOGE(TAG, "Increase JSON capacity or reduce maxNo parameter");
        }
        http.end();
        server.send(200, "application/json", "[]");
        return;
    }

    http.end();

    // Build compact output with only id and name
    DynamicJsonDocument docOut(1024); // Reduced from 2048
    JsonArray outArray = docOut.to<JsonArray>();

    JsonArray locations = docIn["stopLocationOrCoordLocation"];
    for (JsonVariantConst item : locations) {
        JsonObjectConst stopLoc = item["StopLocation"];
        if (!stopLoc.isNull()) {
            JsonObject stop = outArray.createNestedObject();
            stop["name"] = stopLoc["name"].as<String>();
            stop["id"] = stopLoc["id"].as<String>();
        }
    }

    String out;
    serializeJson(docOut, out);
    ESP_LOGI(TAG, "Found %d stations", outArray.size());
    server.send(200, "application/json", out);
}

// AJAX handler for lazy-loading location + nearby stops (GET /api/init)
// Called by browser on page load to avoid blocking the initial HTTP response
void handleInit(WebServer& server) {
    ESP_LOGI(TAG, "Handling /api/init - detecting location and nearby stops");

    ConfigPageData& pageData = ConfigPageData::getInstance();

    // Detect location via Google Geolocation API
    float lat, lon;
    bool locationOk = getLocationFromGoogle(lat, lon);

    String cityName = "";
    if (locationOk) {
        cityName = getCityFromLatLon(lat, lon);
        if (cityName.isEmpty()) cityName = "Unknown";
        pageData.setLocation(lat, lon, cityName);

        // Fetch nearby stops
        getNearbyStops(lat, lon);
    }

    // Build JSON response
    DynamicJsonDocument doc(2048);
    doc["lat"] = locationOk ? lat : 0.0f;
    doc["lon"] = locationOk ? lon : 0.0f;
    doc["city"] = cityName;

    JsonArray stopsArr = doc.createNestedArray("stops");
    for (size_t i = 0; i < pageData.getStopCount(); ++i) {
        JsonObject stop = stopsArr.createNestedObject();
        stop["id"] = pageData.getStopId(i);
        stop["name"] = pageData.getStopName(i);
        stop["dist"] = pageData.getStopDistance(i);
    }

    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
    ESP_LOGI(TAG, "/api/init complete: city=%s, stops=%d", cityName.c_str(), pageData.getStopCount());
}

// Global server reference for callback access
static WebServer* g_server = nullptr;

// Callback wrapper functions
void handleConfigPageWrapper() {
    handleConfigPage(*g_server);
}

void handleSaveConfigWrapper() {
    handleSaveConfig(*g_server);
}

void handleCityAutocompleteWrapper() {
    handleCityAutocomplete(*g_server);
}

void handleStopAutocompleteWrapper() {
    handleStopAutocomplete(*g_server);
}

void handleInitWrapper() {
    handleInit(*g_server);
}

void setupWebServer(WebServer& server) {
    ESP_LOGI(TAG, "Setting up web server...");
    g_server = &server;

    // Pre-render config page once (avoids slow char-by-char template processing per request)
    g_renderedPage = renderConfigPageHtml();
    ESP_LOGI(TAG, "Config page pre-rendered: %d bytes", g_renderedPage.length());

    server.on("/", handleConfigPageWrapper);
    server.on("/save_config", HTTP_POST, handleSaveConfigWrapper);
    server.on("/api/city", HTTP_GET, handleCityAutocompleteWrapper);
    server.on("/api/stop", HTTP_GET, handleStopAutocompleteWrapper);
    server.on("/api/init", HTTP_GET, handleInitWrapper);
    server.begin();
    ESP_LOGI("WEB_SERVER", "HTTP server started.");
}
