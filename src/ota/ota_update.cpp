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


// Check update task
// downloads every 30sec the json file with the latest firmware
void check_update_task(void* pvParameter) {
    while (1) {
        // Connect to WiFi in station mode
        MyWiFiManager::reconnectWiFi();
        // Check if WiFi is connected before attempting HTTP request
        if (WiFi.status() != WL_CONNECTED) {
            ESP_LOGW(TAG, "WiFi not connected, skipping OTA check");
            vTaskDelay(30000 / portTICK_PERIOD_MS);
            continue;
        }

        printf("Looking for a new firmware...\n");

        // configure the esp_http_client
        esp_http_client_config_t config = {
            .url = UPDATE_JSON_URL,
            .cert_pem = server_cert_pem_start,
            // .cert_len = server_cert_pem_end - server_cert_pem_start,
            .timeout_ms = 15000,
            .event_handler = _http_event_handler,
        };
        esp_http_client_handle_t client = esp_http_client_init(&config);

        // downloading the json file
        esp_err_t err = esp_http_client_perform(client);
        if (err == ESP_OK) {
            // parse the json file
            cJSON* json = cJSON_Parse(rcv_buffer);
            if (json == NULL) printf("downloaded file is not a valid json, aborting...\n");
            else {
                cJSON* version = cJSON_GetObjectItemCaseSensitive(json, "version");
                cJSON* file = cJSON_GetObjectItemCaseSensitive(json, "file");

                // check the version
                if (!cJSON_IsNumber(version)) printf("unable to read new version, aborting...\n");
                else {
                    double new_version = version->valuedouble;
                    if (new_version > 0.2) {
                        printf("current firmware version (%.1f) is lower than the available one (%.1f), upgrading...\n",
                               FIRMWARE_VERSION, new_version);
                        if (cJSON_IsString(file) && (file->valuestring != NULL)) {
                            printf("downloading and installing new firmware (%s)...\n", file->valuestring);
                            esp_http_client_config_t ota_client_config = {
                                .url = file->valuestring,
                                .cert_pem = server_cert_pem_start,
                                // .cert_len = server_cert_pem_end - server_cert_pem_start,
                                // .timeout_ms = 15000,
                                .event_handler = _http_event_handler,
                            };
                            esp_err_t ret = esp_https_ota(&ota_client_config);
                            if (ret == ESP_OK) {
                                printf("OTA OK, restarting...\n");
                                esp_restart();
                            } else {
                                printf("OTA failed...\n");
                            }
                        } else printf("unable to read the new file name, aborting...\n");
                    } else
                        printf(
                            "current firmware version (%.1f) is greater or equal to the available one (%.1f), nothing to do...\n",
                            FIRMWARE_VERSION, new_version);
                }
            }
        } else printf("unable to download the json file, aborting...\n");

        // cleanup
        esp_http_client_cleanup(client);

        printf("\n");
        vTaskDelay(30000 / portTICK_PERIOD_MS);
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

bool getLatestReleaseFromGitHub(const char* owner, const char* repo, ReleaseInfo& releaseInfo) {
    ESP_LOGI(TAG, "Fetching latest release for %s/%s", owner, repo);

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
