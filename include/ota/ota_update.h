#pragma once
#include <Arduino.h>
#include <esp_http_client.h>

#include "ota/version_helper.h"

// OTA Configuration
#define LATEST_RELEASE_API "https://api.github.com/repos/gogo-boot/mystation/releases/latest"

// Board-specific firmware asset name for OTA
#if defined(PCB_E1001)
    #define OTA_FIRMWARE_ASSET "firmware-e1001.bin"
#elif defined(PCB_EE04)
    #define OTA_FIRMWARE_ASSET "firmware-ee04.bin"
#elif defined(BOARD_ESP32_C3)
    #define OTA_FIRMWARE_ASSET "firmware-c3.bin"
#else
    #define OTA_FIRMWARE_ASSET "firmware.bin"
#endif

// External variables
extern char rcv_buffer[200];
extern const char server_cert_pem_start[] asm("_binary_cert_github_bundle_pem_start");
extern const char server_cert_pem_end[] asm("_binary_cert_github_bundle_pem_end");

// OTA result codes
enum OTAResult {
    OTA_UP_TO_DATE,
    OTA_UPDATE_FAILED
    // Note: successful update reboots the device, so no "success" value needed
};

// Function declarations
esp_err_t _http_event_handler(esp_http_client_event_t* evt);

OTAResult check_ota_update();

// Release information structure
struct ReleaseInfo {
    String tagName;
    SemanticVersion version;
    String firmwareUrl;
};

// New function declaration
bool getLatestReleaseFromGitHub(ReleaseInfo& releaseInfo);
