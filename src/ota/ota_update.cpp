#include "ota/ota_update.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "cJSON.h"
#include "util/wifi_manager.h"
#include <esp_https_ota.h>
#include <HTTPClient.h>
#include <StreamUtils.h>
#include "build_config.h"
#include "ota/version_helper.h"

static const char* TAG = "OTA_UPDATE";

// receive buffer
char rcv_buffer[200];

// Declare the certificate
extern const char server_cert_pem_start[] asm("_binary_cert_github_pem_start");
extern const char server_cert_pem_end[] asm("_binary_cert_github_pem_end");

// esp_http_client event handler
esp_err_t _http_event_handler(esp_http_client_event_t* evt) {
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        break;
    case HTTP_EVENT_ON_CONNECTED:
        break;
    case HTTP_EVENT_HEADER_SENT:
        break;
    case HTTP_EVENT_ON_HEADER:
        break;
    case HTTP_EVENT_ON_DATA:
        if (!esp_http_client_is_chunked_response(evt->client)) {
            strncpy(rcv_buffer, (char*)evt->data, evt->data_len);
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        break;
    case HTTP_EVENT_DISCONNECTED:
        break;
    }
    return ESP_OK;
}

void check_ota_update() {
    ReleaseInfo release;
    if (getLatestReleaseFromGitHub(release)) {
        SemanticVersion current = SemanticVersion::parse(FIRMWARE_VERSION);
        if (release.version.isNewerThan(current)) {
            ESP_LOGI(TAG, "Update available: %s -> %s",
                     current.toString().c_str(),
                     release.version.toString().c_str());
            esp_http_client_config_t ota_client_config = {
                .url = release.firmwareUrl.c_str(),
                .cert_pem = server_cert_pem_start,
                .cert_len = (size_t)(server_cert_pem_end - server_cert_pem_start),
                // .timeout_ms = 15000,
                .max_redirection_count = 5,
                .event_handler = _http_event_handler,
                .buffer_size = 2048,
                .buffer_size_tx = 2048,
                .keep_alive_enable = true,
            };
            esp_err_t ret = esp_https_ota(&ota_client_config);
            if (ret == ESP_OK) {
                printf("OTA OK, restarting...\n");
                esp_restart();
            } else {
                printf("OTA failed...\n");
            }
        }
    }
}


namespace {
    StaticJsonDocument<256> releaseFilter;

    void initReleaseFilter() {
        releaseFilter["tag_name"] = true;
        releaseFilter["assets"][0]["name"] = true;
        releaseFilter["assets"][0]["browser_download_url"] = true;
    }
}

bool getLatestReleaseFromGitHub(ReleaseInfo& releaseInfo) {
    ESP_LOGI(TAG, "Fetching latest release ");

    HTTPClient http;

    String url = LATEST_RELEASE_API;
    ESP_LOGI(TAG, "Requesting: %s", url.c_str());

    http.begin(url);
    http.addHeader("Accept", "application/vnd.github+json");
    http.addHeader("X-GitHub-Api-Version", "2022-11-28");
    // Todo : Add Authorization header
    // http.addHeader("Authorization", "Bearer YOUR-TOKEN");

    const char* keys[] = {"Transfer-Encoding"};
    http.collectHeaders(keys, 1);

    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        ESP_LOGE(TAG, "HTTP GET failed, error: %s", http.errorToString(httpCode).c_str());
        http.end();
        return false;
    }

    initReleaseFilter();

    // Handle chunked encoding
    Stream& rawStream = http.getStream();
    ChunkDecodingStream decodedStream(http.getStream());
    Stream& response = http.header("Transfer-Encoding") == "chunked" ? decodedStream : rawStream;

    const size_t JSON_CAPACITY = 4096;
    DynamicJsonDocument doc(JSON_CAPACITY);
    DeserializationOption::NestingLimit nestingLimit(10);

    DeserializationError error = deserializeJson(
        doc, response, DeserializationOption::Filter(releaseFilter),
        nestingLimit
    );

    if (error) {
        ESP_LOGE(TAG, "JSON parse failed: %s", error.c_str());
        if (error == DeserializationError::NoMemory) {
            ESP_LOGE(TAG, "Increase JSON_CAPACITY (current: %d)", JSON_CAPACITY);
        }
        http.end();
        return false;
    }

    http.end();

    ESP_LOGI(TAG, "Memory used: %u/%u bytes", doc.memoryUsage(), doc.capacity());

    // Parse tag_name
    const char* tag_name = doc["tag_name"];
    if (!tag_name) {
        ESP_LOGE(TAG, "Unable to read tag_name");
        return false;
    }

    releaseInfo.tagName = String(tag_name);
    releaseInfo.version = SemanticVersion::parse(tag_name);

    // Find firmware.bin in assets
    JsonArrayConst assets = doc["assets"];
    bool foundFirmware = false;

    for (JsonVariantConst asset : assets) {
        const char* name = asset["name"];
        if (name && strcmp(name, "firmware.bin") == 0) {
            const char* download_url = asset["browser_download_url"];
            if (download_url) {
                releaseInfo.firmwareUrl = String(download_url);
                foundFirmware = true;
                ESP_LOGI(TAG, "Found firmware: %s", download_url);
                break;
            }
        }
    }

    if (!foundFirmware) {
        ESP_LOGW(TAG, "firmware.bin not found in release assets");
        return false;
    }

    ESP_LOGI(TAG, "Successfully fetched release: %s (version: %s)",
             releaseInfo.tagName.c_str(),
             releaseInfo.version.toString().c_str());

    return true;
}
