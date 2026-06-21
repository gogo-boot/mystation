#include "api/rmv_api.h"
#include "api/rmv_json_parser.h"
#include <HTTPClient.h>
#include <vector>
#include <Arduino.h>
#include "util/util.h"
#include "util/time_manager.h"
#include <esp_log.h>
#include <StreamUtils.h>
#include "config/config_struct.h"
#include "config/config_manager.h"
#include "config/config_page_data.h"
#include "sec/aes_crypto.h"
#include "build_config.h"
#include <time.h>

static const char* TAG = "RMV_API";
// const size_t JSON_CAPACITY = 16384; // 16KB - safer for API responses
const size_t JSON_CAPACITY = 10240; // 10KB - safer for API responses

namespace {
    JsonDocument departureFilter;

    void initDepartureFilter() {
        departureFilter["Departure"][0]["time"] = true;
        departureFilter["Departure"][0]["track"] = true;
        departureFilter["Departure"][0]["rtTime"] = true;
        departureFilter["Departure"][0]["cancelled"] = true;
        departureFilter["Departure"][0]["direction"] = true;
        departureFilter["Departure"][0]["directionFlag"] = true;

        departureFilter["Departure"][0]["Product"][0]["line"] = true;
        departureFilter["Departure"][0]["Product"][0]["catOut"] = true;
        departureFilter["Departure"][0]["Messages"]["Message"][0]["head"] = true;
    }

    // Build RMV products parameter based on filter flags
    String buildProductsFilter(uint8_t filterFlags) {
        if (filterFlags == 0) {
            // No filters - return empty string to get all transport types
            return "";
        }

        // RMV product bit values (from RMV API documentation)
        // These values are passed directly to RMV API as product codes
        int productsBitmask = 0;
        if (filterFlags & FILTER_R) productsBitmask |= FILTER_R; // Regional (4)
        if (filterFlags & FILTER_S) productsBitmask |= FILTER_S; // S-Bahn (8)
        if (filterFlags & FILTER_U) productsBitmask |= FILTER_U; // U-Bahn (16)
        if (filterFlags & FILTER_TRAM) productsBitmask |= FILTER_TRAM; // Tram (32)
        if (filterFlags & FILTER_BUS) productsBitmask |= FILTER_BUS; // Bus (64)
        if (filterFlags & FILTER_HIGHFLOOR) productsBitmask |= FILTER_HIGHFLOOR; // High-floor Bus (128)
        if (filterFlags & FILTER_FERRY) productsBitmask |= FILTER_FERRY; // Ferry (256)
        if (filterFlags & FILTER_CALLBUS) productsBitmask |= FILTER_CALLBUS; // Call Bus (512)

        if (productsBitmask == 0) {
            return ""; // No valid filters
        }

        return "&products=" + String(productsBitmask);
    }

    // Calculate departure time including walking time for RMV API time parameter
    // Uses TimeManager::getCurrentLocalTime() to ensure proper timezone handling
    String calculateDepartureTime(int walkingTimeMinutes) {
        tm timeinfo;
        if (!TimeManager::getCurrentLocalTime(timeinfo)) {
            ESP_LOGE(TAG, "Failed to get current local time for departure calculation");
            return "00:00"; // Fallback - will likely cause API to return current departures
        }

        // Convert tm to time_t for adding walking time
        time_t now = mktime(&timeinfo);
        now += (walkingTimeMinutes * 60);

        // Convert back to local time (timezone already applied by getCurrentLocalTime)
        localtime_r(&now, &timeinfo);

        // Format as HH:MM for RMV API
        char timeStr[6];
        snprintf(timeStr, sizeof(timeStr), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

        ESP_LOGD(TAG, "Calculated departure time: %s (walking time: %d min)", timeStr, walkingTimeMinutes);
        return String(timeStr);
    }
} // end anonymous namespace

void getNearbyStops(float lat, float lon) {
    std::vector<Station> stations;
    Util::printFreeHeap("Before RMV request:");
    HTTPClient http;

    // Use static utility method for secure API key decryption (no caching)
    std::string decrypted = AESCrypto::getRMVAPIKey();

    char url[256];
    snprintf(url, sizeof(url),
        "https://www.rmv.de/hapi/location.nearbystops?accessId=%s&originCoordLat=%.6f&originCoordLong=%.6f&format=json&maxNo=7",
        decrypted.c_str(), lat, lon);
    DEBUG_ONLY(
        String urlForLog = url;
        int keyPos = urlForLog.indexOf("accessId=");
        if (keyPos != -1) {
            int keyEnd = urlForLog.indexOf('&', keyPos);
            if (keyEnd == -1) keyEnd = urlForLog.length();
            urlForLog.replace(urlForLog.substring(keyPos, keyEnd), "accessId=***");
        }
        ESP_LOGI(TAG, "Requesting nearby stops: %s", urlForLog.c_str());
    );
    http.begin(url);
    http.setTimeout(10000);
    int httpCode = http.GET();
    if (httpCode > 0) {
        String payload = http.getString();
        ESP_LOGD(TAG, "Nearby stops response: %s", payload.c_str());
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
            ConfigPageData& pageData = ConfigPageData::getInstance();
            pageData.clearStops();
            JsonArray stops = doc["stopLocationOrCoordLocation"];
            for (JsonObject item : stops) {
                JsonObject stop = item["StopLocation"];
                if (!stop.isNull()) {
                    const char* id = stop["id"] | "";
                    const char* name = stop["name"] | "";
                    float lon = stop["lon"] | 0.0;
                    float lat = stop["lat"] | 0.0;
                    int dist = stop["dist"] | 0;
                    int products = stop["products"] | 0;
                    String type = (products & 64) ? "train" : "bus";
                    stations.push_back({String(id), String(name), type});
                    pageData.addStop(id, name, String(dist));
                    ESP_LOGI(TAG, "Stop ID: %s, Name: %s, Lon: %f, Lat: %f, Type: %s", id, name, lon, lat,
                             type.c_str());
                }
            }
        } else {
            ESP_LOGE(TAG, "Failed to parse RMV JSON: %s", error.c_str());
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET failed, error: %s", http.errorToString(httpCode).c_str());
    }
    http.end();
    Util::printFreeHeap("After RMV request:");
}

// Populate departure data from JSON document
bool populateDepartureData(const JsonDocument& doc, DepartureData& departData) {
    ESP_LOGI(TAG, "Populating departure data from JSON response");

    // Clear existing departures
    departData.departures.clear();
    departData.departureCount = 0;

    // Use JsonArrayConst for const document
    JsonArrayConst departures = doc["Departure"];
    if (departures.size() == 0) {
        ESP_LOGW(TAG, "Departure array is empty");
        return true; // ← Also return true for empty array
    }
    ESP_LOGI(TAG, "Found %d departures in response", departures.size());

    // Reserve capacity to avoid multiple reallocations
    departData.departures.reserve(departures.size());

    for (JsonVariantConst departureVariant : departures) {
        DepartureInfo depInfo;

        /*
        {
          "Departure": [
            {
              "Product": [
                {
                  "line": "S6",
                  "catOut": "Bus"
                }
              ],
              "Messages": {
                "Message": [
                  {
                    "head": "S3, S4, S5: nächtliche Teilausfälle mit Ersatzverkehr"
                  }
                ]
              },
              "time": "22:37:00",
              "rtTime": "22:37:00",
              "cancelled": true,
              "direction": "Frankfurt (Main) Hauptbahnhof Südseite",
              "directionFlag": "1"
            }
        */
        // Extract basic departure information with safe string conversion
        depInfo.direction = safeJsonString(departureVariant, "direction");
        depInfo.directionFlag = safeJsonString(departureVariant, "directionFlag");
        depInfo.time = safeJsonString(departureVariant, "time");
        depInfo.rtTime = safeJsonString(departureVariant, "rtTime");
        depInfo.cancelled = departureVariant["cancelled"].as<bool>();
        depInfo.track = safeJsonString(departureVariant, "track");

        // Extract category from Product array with safety checks
        if (departureVariant["Product"].size() > 0) {
            depInfo.category = safeJsonString(departureVariant["Product"][0], "catOut");
            depInfo.line = safeJsonString(departureVariant["Product"][0], "line");
        }

        // Extract category from Product array with safety checks
        if (departureVariant["Messages"]["Message"].size() > 0) {
            // Direct path access
            const char* head = departureVariant["Messages"]["Message"][0]["head"];
            depInfo.text = head ? String(head) : "";
        }

        // Add to departures vector
        departData.departures.push_back(depInfo);

        ESP_LOGD(TAG, "Added: %s -> %s at %s (RT: %s) [%s]",
                 depInfo.line.c_str(),
                 depInfo.direction.c_str(),
                 depInfo.time.c_str(),
                 depInfo.rtTime.c_str(),
                 depInfo.category.c_str());
    }

    departData.departureCount = static_cast<int>(departData.departures.size());
    ESP_LOGI(TAG, "Successfully populated %d departures", departData.departureCount);

    return true;
}

bool getDepartureFromRMV(const char* stopId, DepartureData& departData) {
    ESP_LOGI(TAG, "Fetching departure data for stop: %s", stopId);

    // Use static utility method for secure API key decryption (no caching)
    std::string decrypted = AESCrypto::getRMVAPIKey();

    HTTPClient http;
    String encodedId = Util::urlEncode(String(stopId));

    // Get configured vehicle type filters from ConfigManager
    RTCConfigData& config = ConfigManager::getConfig();
    std::vector<String> activeFilters = ConfigManager::getActiveFilters();

    // Build products parameter based on active filters
    String productsParam = buildProductsFilter(config.filterFlags);

    // Calculate departure time and date including walking time for API request
    String departureTime = calculateDepartureTime(config.walkingTime);

    char url[512];
    snprintf(url, sizeof(url),
        "https://www.rmv.de/hapi/departureBoard?accessId=%s&id=%s&format=json&maxJourneys=22&duration=90%s&time=%s",
        decrypted.c_str(), encodedId.c_str(), productsParam.c_str(), departureTime.c_str());

    DEBUG_ONLY(
        String urlForLog = url;
        int keyPos = urlForLog.indexOf("accessId=");
        if (keyPos != -1) {
            int keyEnd = urlForLog.indexOf('&', keyPos);
            if (keyEnd == -1) keyEnd = urlForLog.length();
            urlForLog.replace(urlForLog.substring(keyPos, keyEnd), "accessId=***");
        }
        ESP_LOGI(TAG, "Requesting departure board: %s", urlForLog.c_str());
        ESP_LOGI(TAG, "Walking time: %d minutes, departure time filter: %s", config.walkingTime, departureTime.c_str());
    );

    http.begin(url);
    http.setTimeout(10000);

    const char* keys[] = {"Transfer-Encoding"};
    http.collectHeaders(keys, 1);

    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        ESP_LOGE(TAG, "HTTP GET failed, error: %s", http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }

    static bool filterReady = false;
    if (!filterReady) { initDepartureFilter(); filterReady = true; }

    // Create the raw and decoded stream
    Stream& rawStream = http.getStream();
    ChunkDecodingStream decodedStream(http.getStream());

    // Choose the stream based on the Transfer-Encoding header
    Stream& response = http.header("Transfer-Encoding") == "chunked" ? decodedStream : rawStream;

    JsonDocument doc;
    DeserializationOption::NestingLimit nestingLimit(20);

    // Always check for memory errors
    DeserializationError error = deserializeJson(doc, response, DeserializationOption::Filter(departureFilter),
                                                 nestingLimit);

    if (error) {
        ESP_LOGE(TAG, "JSON parse failed: %s", error.c_str());
        if (error == DeserializationError::NoMemory) {
            ESP_LOGE(TAG, "Increase JSON_CAPACITY (current: %d)", JSON_CAPACITY);
        }
        return false;
    }

    http.end();

#if CORE_DEBUG_LEVEL >= 4
    String prettyJson;
    serializeJsonPretty(doc, prettyJson);
    ESP_LOGD(TAG, "JSON Document (pretty):\n%s", prettyJson.c_str());
#endif

    // Check actual memory usage
    ESP_LOGI(TAG, "Memory used: %u bytes, Free heap: %u bytes", doc.memoryUsage(), ESP.getFreeHeap());

    // Set basic departure data
    departData.stopId = String(stopId);

    // Populate departure data from JSON
    if (!populateDepartureData(doc, departData)) {
        ESP_LOGE(TAG, "Failed to populate departure data");
        return false;
    }

    return true;
}


// ============================================================================
// Trip/Connection API
// ============================================================================

static int parseDuration(const char* dur) {
    // Parse ISO 8601 duration: "PT19M" → 19, "PT1H19M" → 79
    int h = 0, m = 0;
    if (sscanf(dur, "PT%dH%dM", &h, &m) == 2) return h * 60 + m;
    if (sscanf(dur, "PT%dM", &m) == 1) return m;
    if (sscanf(dur, "PT%dH", &h) == 1) return h * 60;
    return 0;
}

static void copyTime(char* dest, const char* src, size_t destSize) {
    // Copy "HH:MM:SS" → "HH:MM" (first 5 chars)
    if (src && strlen(src) >= 5) {
        strncpy(dest, src, 5);
        dest[5] = '\0';
    } else {
        dest[0] = '\0';
    }
}

bool getTripFromRMV(const char* originId, const char* destId, TripData& tripData) {
    ESP_LOGI(TAG, "Fetching trip data: %s → %s", originId, destId);

    tripData.connectionCount = 0;

    RTCConfigData& config = ConfigManager::getConfig();
    std::string decrypted = AESCrypto::getRMVAPIKey();

    // Calculate departure time with walking time offset
    time_t now;
    time(&now);
    now += config.walkingTime * 60; // Add walking time
    struct tm* t = localtime(&now);
    char departureTime[6];
    snprintf(departureTime, sizeof(departureTime), "%02d:%02d", t->tm_hour, t->tm_min);

    String encodedOrigin = Util::urlEncode(String(originId));
    String encodedDest = Util::urlEncode(String(destId));

    char url[512];
    snprintf(url, sizeof(url),
        "https://www.rmv.de/hapi/trip?accessId=%s&originId=%s&destId=%s&format=json&numF=6&products=%d&time=%s",
        decrypted.c_str(), encodedOrigin.c_str(), encodedDest.c_str(),
        config.filterFlags, departureTime);

    ESP_LOGI(TAG, "Trip API request (time=%s, products=%d)", departureTime, config.filterFlags);

    HTTPClient http;
    http.begin(url);
    http.setTimeout(10000);

    const char* keys[] = {"Transfer-Encoding"};
    http.collectHeaders(keys, 1);

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        ESP_LOGE(TAG, "Trip HTTP GET failed: %s", http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }

    // JSON filter — only parse the fields we need
    JsonDocument filter;
    filter["Trip"][0]["duration"] = true;
    filter["Trip"][0]["LegList"]["Leg"][0]["type"] = true;
    filter["Trip"][0]["LegList"]["Leg"][0]["name"] = true;
    filter["Trip"][0]["LegList"]["Leg"][0]["direction"] = true;
    filter["Trip"][0]["LegList"]["Leg"][0]["Origin"]["time"] = true;
    filter["Trip"][0]["LegList"]["Leg"][0]["Origin"]["rtTime"] = true;
    filter["Trip"][0]["LegList"]["Leg"][0]["Origin"]["track"] = true;
    filter["Trip"][0]["LegList"]["Leg"][0]["Destination"]["time"] = true;
    filter["Trip"][0]["LegList"]["Leg"][0]["Destination"]["rtTime"] = true;

    // Stream and parse
    Stream& rawStream = http.getStream();
    ChunkDecodingStream decodedStream(http.getStream());
    Stream& response = http.header("Transfer-Encoding") == "chunked" ? decodedStream : rawStream;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response,
        DeserializationOption::Filter(filter),
        DeserializationOption::NestingLimit(20));

    http.end();

    if (error) {
        ESP_LOGE(TAG, "Trip JSON parse failed: %s", error.c_str());
        return false;
    }

    // Parse trips
    JsonArray trips = doc["Trip"];
    for (JsonObject trip : trips) {
        if (tripData.connectionCount >= 6) break;

        TripConnection& conn = tripData.connections[tripData.connectionCount];
        conn.legCount = 0;
        conn.durationMinutes = parseDuration(trip["duration"] | "PT0M");

        JsonArray legs = trip["LegList"]["Leg"];
        for (JsonObject leg : legs) {
            if (conn.legCount >= 4) break;

            // Skip walking legs
            const char* type = leg["type"] | "";
            if (strcmp(type, "JNY") != 0) continue;

            TripLeg& tl = conn.legs[conn.legCount];

            copyTime(tl.departureTime, leg["Origin"]["time"] | "", sizeof(tl.departureTime));
            copyTime(tl.arrivalTime, leg["Destination"]["time"] | "", sizeof(tl.arrivalTime));
            copyTime(tl.rtDepartureTime, leg["Origin"]["rtTime"] | "", sizeof(tl.rtDepartureTime));
            copyTime(tl.rtArrivalTime, leg["Destination"]["rtTime"] | "", sizeof(tl.rtArrivalTime));

            strncpy(tl.line, leg["name"] | "", sizeof(tl.line) - 1);
            tl.line[sizeof(tl.line) - 1] = '\0';

            strncpy(tl.direction, leg["direction"] | "", sizeof(tl.direction) - 1);
            tl.direction[sizeof(tl.direction) - 1] = '\0';

            const char* track = leg["Origin"]["track"] | "";
            strncpy(tl.platform, track, sizeof(tl.platform) - 1);
            tl.platform[sizeof(tl.platform) - 1] = '\0';

            conn.legCount++;
        }

        if (conn.legCount > 0) {
            tripData.connectionCount++;
            ESP_LOGI(TAG, "  Connection %d: %s→%s (%d min, %d legs)",
                tripData.connectionCount,
                conn.legs[0].departureTime,
                conn.legs[conn.legCount - 1].arrivalTime,
                conn.durationMinutes,
                conn.legCount);
        }
    }

    ESP_LOGI(TAG, "Trip: found %d connections", tripData.connectionCount);
    return tripData.connectionCount > 0;
}
