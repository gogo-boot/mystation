#pragma once
#include <Arduino.h>
#include <esp_http_client.h>

#include "ota/version_helper.h"

// OTA Configuration
#define UPDATE_JSON_URL     "https://raw.githubusercontent.com/gogo-boot/mystation/refs/heads/61-firmware-ota-update/test/ota/example.json"
#define LATEST_RELEASE_API "https://api.github.com/repos/gogo-boot/mystation/releases/latest"

// External variables
extern char rcv_buffer[200];
extern const char server_cert_pem_start[] asm("_binary_cert_github_pem_start");
extern const char server_cert_pem_end[] asm("_binary_cert_github_pem_end");

// Function declarations
esp_err_t _http_event_handler(esp_http_client_event_t* evt);

void check_ota_update();

// Release information structure
struct ReleaseInfo {
    String tagName;
    SemanticVersion version;
    String firmwareUrl;
};

// New function declaration
bool getLatestReleaseFromGitHub(ReleaseInfo& releaseInfo);
