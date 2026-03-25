# ESP32 Flash Encryption Guide

This guide explains how to enable flash encryption to protect your firmware and API keys from being extracted.

## Overview

ESP32 Flash Encryption uses AES-256 hardware encryption to protect:

- Firmware code
- Embedded strings (including API keys)
- LittleFS/SPIFFS data
- NVS data (optional)

**Once enabled in Release Mode, it CANNOT be disabled!**

## Prerequisites

- ESP-IDF framework (Arduino framework has limited encryption support)
- Understanding of ESP32 eFuse (one-time programmable)
- Serial connection for initial setup

## Quick Comparison

| Aspect                | Without Encryption   | With Encryption         |
|-----------------------|----------------------|-------------------------|
| Flash read protection | ❌ Anyone can dump    | ✅ Encrypted data only   |
| API keys visible      | ❌ Yes, via `strings` | ✅ No, encrypted         |
| OTA updates           | Simple               | Requires pre-encryption |
| Development ease      | Easy                 | More complex            |
| Reversible            | N/A                  | ❌ NO (Release mode)     |

## Method 1: Arduino Framework (Limited Support)

Arduino framework has limited built-in support for flash encryption. You can enable it via `menuconfig` but it requires
ESP-IDF components.

### Steps:

1. Add to `platformio.ini`:

```ini
[env:esp32-s3-encrypted]
platform = espressif32
framework = arduino
board = seeed_xiao_esp32s3
board_build.flash_mode = dio
board_build.partitions = default_16MB.csv

; Enable flash encryption via sdkconfig
board_build.cmake_extra_args =
    -DCONFIG_SECURE_FLASH_ENC_ENABLED=y
    -DCONFIG_SECURE_FLASH_ENCRYPTION_MODE_DEVELOPMENT=y
```

2. First flash must be done via serial (not OTA)

## Method 2: ESP-IDF Framework (Full Support)

For full flash encryption support, migrate to ESP-IDF framework.

### 1. Update platformio.ini

```ini
[env:esp32-s3-encrypted]
platform = espressif32
framework = espidf
board = seeed_xiao_esp32s3

; Custom sdkconfig for encryption
board_build.cmake_extra_args = -DSDKCONFIG_DEFAULTS="sdkconfig.defaults"
```

### 2. Create sdkconfig.defaults

```
# Flash Encryption (Development Mode - can be disabled)
CONFIG_SECURE_FLASH_ENC_ENABLED=y
CONFIG_SECURE_FLASH_ENCRYPTION_MODE_DEVELOPMENT=y

# For Release Mode (PERMANENT - CANNOT BE UNDONE!)
# CONFIG_SECURE_FLASH_ENCRYPTION_MODE_RELEASE=y

# Encrypt partitions
CONFIG_SECURE_ENCRYPT_FLASH_DATA=y

# Optional: Secure Boot (additional protection)
# CONFIG_SECURE_BOOT=y
```

### 3. First-Time Flash (Serial Only)

```bash
# Build with encryption support
pio run -e esp32-s3-encrypted

# Flash via serial - encryption keys will be generated and burned to eFuse
pio run -e esp32-s3-encrypted -t upload
```

## OTA with Flash Encryption

When flash encryption is enabled, OTA updates require **pre-encrypted** firmware.

### Option A: Device-Side Encryption (Simpler)

The ESP32 can encrypt received OTA data before writing to flash. This works automatically if:

- Flash encryption is enabled
- Using `esp_https_ota` or `esp_ota_ops` APIs (which you already use)

**Your current OTA code should work!** The ESP32 encrypts data as it writes to flash.

### Option B: Pre-Encrypted OTA (More Secure)

For maximum security, encrypt firmware before distribution:

```bash
# Generate encryption key (store securely!)
espsecure.py generate_flash_encryption_key my_flash_key.bin

# Encrypt firmware before upload
espsecure.py encrypt_flash_data \
    --keyfile my_flash_key.bin \
    --address 0x10000 \
    --output firmware_encrypted.bin \
    firmware.bin
```

**Note**: This requires all devices to use the same encryption key, which is less secure than per-device keys.

## Checking Encryption Status

```bash
# Check if flash encryption is enabled
espefuse.py -p /dev/cu.usbmodem* summary | grep FLASH_CRYPT
```

## Important Warnings

### ⚠️ Development vs Release Mode

| Mode        | Can Disable?        | Use Case             |
|-------------|---------------------|----------------------|
| Development | Yes (limited times) | Testing, development |
| Release     | **NO - PERMANENT**  | Production devices   |

### ⚠️ eFuse Burns are Permanent

- eFuse can only be written once
- Wrong configuration = bricked device
- Test thoroughly in Development mode first

### ⚠️ Key Management

- Each ESP32 generates its own key (secure)
- Or you can provide a key (for pre-encrypted OTA)
- Lost key = cannot update device

## Recommended Approach for MyStation

Given your current setup with Arduino framework and OTA requirements:

### Phase 1: Keep Current Approach (Recommended for now)

- Your AES encryption of API keys in source code is reasonable
- Focus on fixing the GitHub Actions secrets issue first
- API keys are obfuscated (not plaintext)

### Phase 2: Add Flash Encryption (Future)

1. Test with Development mode on a spare ESP32
2. Verify OTA still works with encryption
3. Only then consider Release mode for production

## Alternative: Secure Element

For highest security without flash encryption complexity:

- Use ESP32-S3 with ATECC608 secure element
- Store API keys in tamper-resistant hardware
- More expensive but more flexible

## References

- [ESP-IDF Flash Encryption](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/security/flash-encryption.html)
- [ESP32-S3 Security Features](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/security/security.html)
- [PlatformIO ESP32 Encryption](https://docs.platformio.org/en/latest/platforms/espressif32.html#flash-encryption)

