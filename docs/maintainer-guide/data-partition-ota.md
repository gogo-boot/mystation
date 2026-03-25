# Data Partition OTA Update

This document explains how to implement OTA updates for the LittleFS data partition (SPIFFS), complementing the existing
firmware OTA update functionality.

## Overview

The current OTA implementation only updates the firmware (`firmware.bin`). However, the device also has a data partition
(LittleFS/SPIFFS) that stores files like `config_my_station.html`. This document outlines how to extend OTA to also
update this data partition.

### Current State

| Component      | OTA Support | File           |
|----------------|-------------|----------------|
| Firmware (app) | ✅ Yes       | `firmware.bin` |
| Data Partition | ❌ No        | `littlefs.bin` |

### Target State

| Component      | OTA Support | File           |
|----------------|-------------|----------------|
| Firmware (app) | ✅ Yes       | `firmware.bin` |
| Data Partition | ✅ Yes       | `littlefs.bin` |

## Understanding ESP32 Update Types

ESP32 Arduino/ESP-IDF provides two update command types:

```cpp
#include <Update.h>

// For firmware updates (writes to app partition)
Update.begin(size, U_FLASH);

// For filesystem updates (writes to SPIFFS/LittleFS partition)
Update.begin(size, U_SPIFFS);
```

**Key Difference:**

- `U_FLASH` (default) → Writes to the next OTA app partition (`app0` or `app1`)
- `U_SPIFFS` → Writes to the `spiffs` partition (also works for LittleFS)

> **Note:** Despite the name `U_SPIFFS`, this command works for LittleFS as well since both use the same partition type
> (`data, spiffs`).

## Architecture

### Update Flow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           OTA Update Process                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  1. Check GitHub Release API                                                │
│     └── GET /repos/gogo-boot/mystation/releases/latest                      │
│                                                                             │
│  2. Parse Release Assets                                                    │
│     ├── firmware.bin     → Firmware binary                                  │
│     └── littlefs.bin     → Data partition binary (NEW)                      │
│                                                                             │
│  3. Version Comparison                                                      │
│     ├── Compare firmware version (tag_name vs FIRMWARE_VERSION)             │
│     └── Compare filesystem version (TBD - see Versioning Strategy)          │
│                                                                             │
│  4. Download & Flash (if update needed)                                     │
│     ├── Firmware Update:                                                    │
│     │   └── Update.begin(size, U_FLASH) → app0/app1 partition               │
│     └── Filesystem Update:                                                  │
│         └── Update.begin(size, U_SPIFFS) → spiffs partition                 │
│                                                                             │
│  5. Reboot                                                                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Partition Layout Reference

From `partition/4MB-ota.csv`:

```
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x180000,
app1,     app,  ota_1,   0x190000,0x180000,
spiffs,   data, spiffs,  0x310000,0x5000,      ← LittleFS partition (20KB)
coredump, data, coredump,0x315000,0xEB000,
```

The `spiffs` partition at offset `0x310000` is where LittleFS data is stored. When using `U_SPIFFS`, the ESP32 Update
library automatically finds this partition.

## GitHub Actions - Build LittleFS Binary

### Current Workflow Status

The GitHub Actions workflow (`.github/workflows/build-firmware.yml`) already generates the LittleFS binary during build:

```yaml
# PlatformIO automatically builds littlefs.bin when using:
board_build.filesystem = littlefs
```

The `littlefs.bin` is created at: `.pio/build/{environment}/littlefs.bin`

### Required Changes to GitHub Actions

Update the release job to include `littlefs.bin` in the release assets:

```yaml
# .github/workflows/build-firmware.yml

-   name: Build filesystem image
    run: |
        pio run -e ${{ matrix.environment }} -t buildfs

-   name: Create Release
    uses: softprops/action-gh-release@v2
    with:
        files: |
            # ... existing files ...
            release/firmware.bin
            release/littlefs.bin              # ← Add this line
```

### Expected Release Assets After Change

```json
{
    "tag_name": "v0.6.0",
    "assets": [
        {
            "name": "firmware.bin",
            "browser_download_url": "https://github.com/gogo-boot/mystation/releases/download/v0.6.0/firmware.bin"
        },
        {
            "name": "littlefs.bin",
            "browser_download_url": "https://github.com/gogo-boot/mystation/releases/download/v0.6.0/littlefs.bin"
        }
    ]
}
```

## Implementation Plan

### Phase 1: Update ReleaseInfo Structure

Extend the `ReleaseInfo` struct to include filesystem URL:

```cpp
// include/ota/ota_update.h

struct ReleaseInfo {
    String tagName;
    SemanticVersion version;
    String firmwareUrl;
    String filesystemUrl;    // NEW: URL to littlefs.bin
    bool hasFilesystem;      // NEW: Flag if littlefs.bin exists in release
};
```

### Phase 2: Update GitHub API Parser

Modify `getLatestReleaseFromGitHub()` to also look for `littlefs.bin`:

```cpp
// src/ota/ota_update.cpp - in getLatestReleaseFromGitHub()

for (JsonVariantConst asset : assets) {
    const char* name = asset["name"];
    const char* download_url = asset["browser_download_url"];

    if (name && download_url) {
        if (strcmp(name, "firmware.bin") == 0) {
            releaseInfo.firmwareUrl = String(download_url);
            foundFirmware = true;
        }
        // NEW: Check for littlefs.bin
        else if (strcmp(name, "littlefs.bin") == 0) {
            releaseInfo.filesystemUrl = String(download_url);
            releaseInfo.hasFilesystem = true;
        }
    }
}
```

### Phase 3: Implement Filesystem Update Function

Create a new function to update the data partition:

```cpp
// src/ota/ota_update.cpp

#include <Update.h>

bool performFilesystemUpdate(const char* url) {
    // 1. Resolve redirect URL (same as firmware)
    char directUrl[2048] = {};
    if (!resolveFirmwareUrl(url, directUrl, sizeof(directUrl))) {
        ESP_LOGE(TAG, "Failed to resolve filesystem URL");
        return false;
    }

    // 2. Download the binary
    HTTPClient http;
    http.begin(directUrl);
    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        ESP_LOGE(TAG, "HTTP GET failed: %d", httpCode);
        http.end();
        return false;
    }

    int contentLength = http.getSize();
    WiFiClient* stream = http.getStreamPtr();

    // 3. Begin update with U_SPIFFS type
    if (!Update.begin(contentLength, U_SPIFFS)) {
        ESP_LOGE(TAG, "Not enough space for filesystem update");
        http.end();
        return false;
    }

    // 4. Write data
    size_t written = Update.writeStream(*stream);

    if (written != contentLength) {
        ESP_LOGE(TAG, "Written only %d/%d bytes", written, contentLength);
        http.end();
        return false;
    }

    // 5. Finish update
    if (!Update.end()) {
        ESP_LOGE(TAG, "Update.end() failed: %s", Update.errorString());
        http.end();
        return false;
    }

    http.end();
    ESP_LOGI(TAG, "Filesystem update successful");
    return true;
}
```

### Phase 4: Update Main OTA Logic

Modify `check_ota_update()` to handle both updates:

```cpp
void check_ota_update() {
    ReleaseInfo release;
    if (!getLatestReleaseFromGitHub(release)) {
        return;
    }

    SemanticVersion current = SemanticVersion::parse(FIRMWARE_VERSION);
    bool firmwareUpdateNeeded = release.version.isNewerThan(current);

    // Check if filesystem update is needed
    // (could be based on version or just always update with firmware)
    bool filesystemUpdateNeeded = release.hasFilesystem && firmwareUpdateNeeded;

    if (!firmwareUpdateNeeded && !filesystemUpdateNeeded) {
        ESP_LOGI(TAG, "Everything is up to date");
        return;
    }

    // Update filesystem first (if available)
    // Reason: If firmware update succeeds but filesystem fails,
    // the new firmware might expect new filesystem content
    if (filesystemUpdateNeeded) {
        ESP_LOGI(TAG, "Updating filesystem...");
        if (!performFilesystemUpdate(release.filesystemUrl.c_str())) {
            ESP_LOGE(TAG, "Filesystem update failed");
            // Continue with firmware update anyway? Or abort?
            // Decision: Continue - firmware should handle missing/old files gracefully
        }
    }

    // Update firmware
    if (firmwareUpdateNeeded) {
        ESP_LOGI(TAG, "Updating firmware...");
        // ... existing firmware update code ...
    }

    // Reboot after all updates
    ESP_LOGI(TAG, "OTA complete - rebooting...");
    delay(500);
    esp_restart();
}
```

## Versioning Strategy for Filesystem

### Option 1: Coupled Versioning (Recommended for Start)

Filesystem version is tied to firmware version. When firmware updates, filesystem also updates.

**Pros:**

- Simple to implement
- No additional version tracking needed
- Ensures consistency between firmware and filesystem

**Cons:**

- May update filesystem unnecessarily
- Wastes bandwidth if only firmware changed

### Option 2: Independent Versioning

Store filesystem version in a separate file (e.g., `data/fs_version.txt`) or in NVS.

```cpp
// Check filesystem version
String currentFsVersion = readFsVersionFromNvs();
String newFsVersion = release.filesystemVersion;  // From manifest file

bool filesystemUpdateNeeded = SemanticVersion::parse(newFsVersion)
    .isNewerThan(SemanticVersion::parse(currentFsVersion));
```

**Pros:**

- Only updates when necessary
- Saves bandwidth

**Cons:**

- More complex to implement
- Requires manifest file in release

### Option 3: Hash-Based Versioning

Store MD5/SHA256 hash of filesystem content.

```cpp
// Compare hashes
String currentHash = calculateFsHash();
String releaseHash = release.filesystemHash;  // From release manifest

bool filesystemUpdateNeeded = (currentHash != releaseHash);
```

## Error Handling Considerations

### Filesystem Update Failure Scenarios

| Scenario                     | Current Firmware | New Firmware | Filesystem | Recovery                      |
|------------------------------|------------------|--------------|------------|-------------------------------|
| FS update fails, FW succeeds | ❌                | ✅            | Old        | FW should handle gracefully   |
| FS update succeeds, FW fails | ✅                | ❌            | New        | May have compatibility issues |
| Both fail                    | ✅                | ❌            | Old        | No change, retry later        |
| Both succeed                 | ❌                | ✅            | New        | ✅ Ideal state                 |

### Recommended Update Order

1. **Filesystem First, Then Firmware**
    - If filesystem fails → abort entire update (old firmware + old filesystem = consistent)
    - If filesystem succeeds but firmware fails → potential issue (old firmware + new filesystem)

2. **Alternative: Firmware First, Then Filesystem**
    - If firmware fails → no change (safe)
    - If firmware succeeds but filesystem fails → new firmware + old filesystem (FW must be backward compatible)

### Recommendation

Use **Filesystem First** approach with abort-on-failure for critical filesystem changes.

## Testing Checklist

### Local Testing

- [ ] Build `littlefs.bin` locally: `pio run -t buildfs`
- [ ] Flash `littlefs.bin` manually: `pio run -t uploadfs`
- [ ] Verify files exist on device after flash
- [ ] Test `Update.begin(size, U_SPIFFS)` writes to correct partition

### Integration Testing

- [ ] Create test release with both `firmware.bin` and `littlefs.bin`
- [ ] Verify OTA detects `littlefs.bin` in release assets
- [ ] Test filesystem-only update (when only filesystem changed)
- [ ] Test firmware-only update (when filesystem unchanged)
- [ ] Test combined update (both changed)
- [ ] Test rollback scenario (what happens if update fails mid-way)

### Edge Cases

- [ ] `littlefs.bin` missing from release → firmware-only update
- [ ] Filesystem partition too small for new `littlefs.bin`
- [ ] Network interruption during filesystem download
- [ ] Corrupted `littlefs.bin` (checksum validation)

## Security Considerations

1. **HTTPS Required**: All downloads must use HTTPS with certificate verification
2. **Checksum Validation**: Consider adding SHA256 hash verification for downloaded binaries
3. **Rollback Protection**: Ensure failed updates don't brick the device

## Files to Modify

| File                                   | Changes                                                                                         |
|----------------------------------------|-------------------------------------------------------------------------------------------------|
| `include/ota/ota_update.h`             | Add `filesystemUrl` and `hasFilesystem` to `ReleaseInfo`                                        |
| `src/ota/ota_update.cpp`               | Add `performFilesystemUpdate()`, modify `getLatestReleaseFromGitHub()` and `check_ota_update()` |
| `.github/workflows/build-firmware.yml` | Add `pio run -t buildfs`, include `littlefs.bin` in release                                     |

## Quick Reference: ESP32 Update API

```cpp
#include <Update.h>

// Check available space
size_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;

// Begin update
Update.begin(updateSize, U_FLASH);   // Firmware
Update.begin(updateSize, U_SPIFFS);  // Filesystem

// Write data
Update.write(data, len);
// or
Update.writeStream(stream);

// Finish
Update.end();
Update.end(true);  // Set as bootable (for firmware)

// Error handling
if (Update.hasError()) {
    Serial.println(Update.errorString());
}

// Progress callback
Update.onProgress([](size_t progress, size_t total) {
    Serial.printf("Progress: %d%%\n", (progress * 100) / total);
});
```

## References

- [ESP32 Arduino Update Library](https://github.com/espressif/arduino-esp32/blob/master/libraries/Update/src/Update.h)
- [ESP-IDF OTA Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ota.html)
- [PlatformIO Filesystem](https://docs.platformio.org/en/latest/platforms/espressif32.html#uploading-files-to-file-system)
- [Existing OTA Update Documentation](./ota-update.md)

