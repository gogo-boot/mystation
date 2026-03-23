#include "ota/ota_update.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "cJSON.h"
#include "util/wifi_manager.h"
#include <esp_https_ota.h>
#include <esp_ota_ops.h>
#include <HTTPClient.h>
#include <StreamUtils.h>
#include "build_config.h"
#include "ota/version_helper.h"

static const char* TAG = "OTA_UPDATE";

// receive buffer
char rcv_buffer[200];

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

/**
 * GitHub asset download URLs (browser_download_url) redirect via HTTP 302 to
 * release-assets.githubusercontent.com with a short-lived signed token in the URL.
 *
 * esp_https_ota_begin() calls esp_ota_begin() internally BEFORE the redirect is
 * followed. When the redirect fires mid-stream the OTA flash-write handle is
 * invalidated → "esp_ota_ops: not found the handle" → ESP_ERR_INVALID_ARG.
 *
 * Fix: issue a HEAD request to the GitHub download URL with max_redirection_count=0
 * so it stops at the 302. Read the Location header directly — that is the signed
 * release-assets URL. Hand that direct URL to esp_https_ota so it never redirects.
 *
 * NOTE: esp_http_client_get_url() does NOT return the redirect destination; it
 * returns the URL the client was initialised with. The Location header must be
 * captured via the event handler.
 */

// Buffer filled by the event handler below during URL resolution
// Must be large enough for GitHub's signed release-assets URLs which can exceed 700 chars
static char s_redirectUrl[1024] = {};

static esp_err_t resolveRedirectEventHandler(esp_http_client_event_t* evt) {
    if (evt->event_id == HTTP_EVENT_ON_HEADER) {
        // Case-insensitive compare for "location"
        if (strcasecmp(evt->header_key, "location") == 0 && evt->header_value) {
            strncpy(s_redirectUrl, evt->header_value, sizeof(s_redirectUrl) - 1);
            s_redirectUrl[sizeof(s_redirectUrl) - 1] = '\0';
        }
    }
    return ESP_OK;
}

static bool resolveFirmwareUrl(const char* originalUrl, char* outUrl, size_t outSize) {
    memset(s_redirectUrl, 0, sizeof(s_redirectUrl));

    esp_http_client_config_t config = {};
    config.url = originalUrl;
    config.cert_pem = server_cert_pem_start;
    config.cert_len = (size_t)(server_cert_pem_end - server_cert_pem_start);
    config.timeout_ms = 15000;
    config.max_redirection_count = 0; // stop at the 302, do NOT follow it
    config.skip_cert_common_name_check = true;
    config.event_handler = resolveRedirectEventHandler;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "resolveFirmwareUrl: failed to init HTTP client");
        return false;
    }

    // Use GET (not HEAD) because GitHub may respond 200 to HEAD but 302 to GET
    // for release asset downloads. We won't read the body — just capture headers.
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    // Open connection and send request, but don't fetch body
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "resolveFirmwareUrl: failed to open connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }
    // Fetch response headers (this triggers the event handler for Location header)
    int content_length = esp_http_client_fetch_headers(client);
    int status = esp_http_client_get_status_code(client);
    (void)content_length; // used only for logging

    // Close without reading body
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    ESP_LOGI(TAG, "resolveFirmwareUrl: status=%d, content_length=%d, location=%s",
             status, content_length, strlen(s_redirectUrl) > 0 ? s_redirectUrl : "(none)");

    // GitHub release assets return 302 redirect to release-assets.githubusercontent.com
    if ((status == 302 || status == 301) && strlen(s_redirectUrl) > 0) {
        strncpy(outUrl, s_redirectUrl, outSize - 1);
        outUrl[outSize - 1] = '\0';
        ESP_LOGI(TAG, "Resolved firmware URL (%d→Location): %s", status, outUrl);
        return true;
    }

    // If we got 200, the URL might already be a direct link (unlikely for GitHub)
    if (status == 200) {
        strncpy(outUrl, originalUrl, outSize - 1);
        outUrl[outSize - 1] = '\0';
        ESP_LOGW(TAG, "No redirect (status 200) – using original URL: %s", outUrl);
        return true;
    }

    ESP_LOGE(TAG, "resolveFirmwareUrl: unexpected status=%d", status);
    return false;
}


void check_ota_update() {
    ReleaseInfo release;
    if (!getLatestReleaseFromGitHub(release)) {
        return;
    }

    SemanticVersion current = SemanticVersion::parse(FIRMWARE_VERSION);
#if OTA_FORCE_UPDATE
    ESP_LOGW(TAG, "OTA_FORCE_UPDATE is enabled – skipping version check. Remote: %s, Local: %s",
             release.version.toString().c_str(), current.toString().c_str());
    bool doUpdate = true;
#else
    bool doUpdate = release.version.isNewerThan(current);
#endif

    if (!doUpdate) {
        ESP_LOGI(TAG, "Firmware is up to date (%s)", current.toString().c_str());
        return;
    }

    ESP_LOGI(TAG, "Update available: %s -> %s",
             current.toString().c_str(), release.version.toString().c_str());

    // Pre-resolve the GitHub 302 redirect to the direct signed release-assets URL
    // so esp_https_ota never encounters a redirect mid-stream (which invalidates
    // the internal OTA flash-write handle and causes ESP_ERR_INVALID_ARG).
    char directUrl[1024] = {};
    if (!resolveFirmwareUrl(release.firmwareUrl.c_str(), directUrl, sizeof(directUrl))) {
        // Fall back to original URL; esp_https_ota will try and likely fail on redirect
        strncpy(directUrl, release.firmwareUrl.c_str(), sizeof(directUrl) - 1);
        ESP_LOGW(TAG, "URL resolution failed – using original URL (may fail on redirect)");
    }

    esp_http_client_config_t ota_client_config = {};
    ota_client_config.url = directUrl;
    ota_client_config.cert_pem = server_cert_pem_start;
    ota_client_config.cert_len = (size_t)(server_cert_pem_end - server_cert_pem_start);
    ota_client_config.timeout_ms = 60000;
    ota_client_config.max_redirection_count = 0;
    ota_client_config.buffer_size = 4096;
    ota_client_config.buffer_size_tx = 2048;
    ota_client_config.skip_cert_common_name_check = true;
    ota_client_config.keep_alive_enable = false;

    ESP_LOGI(TAG, "Starting OTA download from: %s", directUrl);

    // Use direct OTA API for full control over the process
    esp_http_client_handle_t client = esp_http_client_init(&ota_client_config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to init HTTP client for OTA");
        return;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return;
    }

    int content_length = esp_http_client_fetch_headers(client);
    int status_code = esp_http_client_get_status_code(client);
    ESP_LOGI(TAG, "HTTP status: %d, content-length: %d", status_code, content_length);

    if (status_code != 200) {
        ESP_LOGE(TAG, "Unexpected HTTP status: %d", status_code);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return;
    }

    if (content_length <= 0) {
        ESP_LOGE(TAG, "Invalid content length: %d", content_length);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return;
    }

    // Get the next OTA partition to write to
    const esp_partition_t* update_partition = esp_ota_get_next_update_partition(nullptr);
    if (!update_partition) {
        ESP_LOGE(TAG, "Failed to get update partition");
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return;
    }
    ESP_LOGI(TAG, "Writing to partition: %s (offset 0x%lx, size 0x%lx)",
             update_partition->label, update_partition->address, update_partition->size);

    // Begin OTA update
    esp_ota_handle_t ota_handle = 0;
    err = esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(err));
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return;
    }
    ESP_LOGI(TAG, "OTA begin OK, handle: 0x%lx", (unsigned long)ota_handle);

    // Read and write in chunks
    char* buffer = (char*)malloc(4096);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate OTA buffer");
        esp_ota_abort(ota_handle);
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return;
    }

    int total_read = 0;
    int read_len;
    while ((read_len = esp_http_client_read(client, buffer, 4096)) > 0) {
        err = esp_ota_write(ota_handle, buffer, read_len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write failed at %d bytes: %s", total_read, esp_err_to_name(err));
            free(buffer);
            esp_ota_abort(ota_handle);
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
            return;
        }
        total_read += read_len;
        if (total_read % 102400 < 4096) {
            // Log every ~100KB
            ESP_LOGI(TAG, "OTA progress: %d / %d bytes (%.1f%%)",
                     total_read, content_length, (float)total_read * 100 / content_length);
        }
    }

    free(buffer);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (read_len < 0) {
        ESP_LOGE(TAG, "HTTP read error: %d", read_len);
        esp_ota_abort(ota_handle);
        return;
    }

    ESP_LOGI(TAG, "Download complete: %d bytes", total_read);

    // Finish OTA
    err = esp_ota_end(ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
        return;
    }

    // Set boot partition
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "OTA OK – rebooting...");
    printf("OTA OK, restarting...\n");
    delay(500);
    esp_restart();
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
