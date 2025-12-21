# OTA Update

If new firmware versions are available, it checks for updates from the GitHub releases of the `gogo-boot/mystation`
repository. It uses the GitHub API to get the latest release information. You can use the following `curl` command to
fetch the latest release information:

## Github Release API Request Example

### Request Example

```curl
curl -L  -H "Accept: application/vnd.github+json" \
-H "Authorization: Bearer <YOUR-TOKEN>" \
-H "X-GitHub-Api-Version: 2022-11-28" \
https://api.github.com/repos/gogo-boot/mystation/releases/latest
```

> Detail : https://docs.github.com/en/rest/releases/releases#get-the-latest-release

### Response Example

The full example API response is stored in `test/ota/github-release-api.json` file.
only the `tag_name` , `name` and the `browser_download_url` of the release information is required for ota update:

```json5
{
    ...
    "tag_name": "v0.3.0",
    "assets": [
        {
            ...
            "name": "firmware.bin",
            ...
            "browser_download_url": "https://github.com/gogo-boot/mystation/releases/download/v0.3.0/firmware.bin"
        },
        ...
    ]
}
```

## Version Comparison

Current Version is defined in the `firmware_version` field of the `platformio.ini` file.
By compile time git tag information is embedded into `include/build_config.h` file.
using the following configuration in the `platformio.ini` file:

```ini
version_flag = -D BUILD_TIME=$UNIX_TIME
               !echo '-D VERSION=\\"'$(git describe --tags)'\\"'
...
```

> Note: It is dynamic value. so compiler compile always new binary even there is no code change.

It gets the latest version from Github release API. It accepts semantic versioning format (semver) for version
comparison. The version strings should be in the format of `MAJOR.MINOR.PATCH`, optionally prefixed with a `v` (e.g.,
`v1.2.3` or `1.2.3`).

If the latest version obtained from the GitHub release is greater than the current firmware version, it indicates newer
firmware available. and executes the OTA update process.

## Certificates

To download the firmware binary from GitHub securely over HTTPS, the device needs to trust the CA certificates file is
hosted on GitHub's release assets domain, which uses SSL/TLS certificates issued by trusted
Certificate, it will be redirected to `release-assets.githubusercontent.com`.
So it needs 2 CA certificates to verify the server certificates of both domains.

### Quick Understading of Certificates.

```text
Root CA (self-signed) ← What you need
↓
Intermediate CA
↓
Server Certificate (github.com)
```

In certificates chain, the Root CA is not included in the server certificate chain sent by the server.
You need to find out the Root CAs `Issuer`

```bash
# Get the root CA name needed for a domain
openssl s_client -connect release-assets.githubusercontent.com:443 </dev/null 2>/dev/null | \
  openssl x509 -noout -issuer | \
  sed 's/.*CN=\([^,]*\).*/\1/'

Sectigo RSA Domain Validation Secure Server CA
```

### Download Root CA Certificates

Download the required Root CA certificates from Browser. it is easist way to get the up-to-date CA certificates.
or visit the root CA provider's website to download the latest root CA certificates.

### Pem file verification

Test if it can verify the server certificates using the downloaded CA certificates:

View Certificates info:

```bash
openssl x509 -in cert/github.pem -noout -issuer -subject
```

Test connection using the CA file:

```bash
openssl s_client -connect api.github.com:443 -CAfile cert/github.pem </dev/null 2>&1 | grep "Verify return code"

openssl s_client -connect release-assets.githubusercontent.com:443 -CAfile cert/github.pem </dev/null 2>&1 | grep "Verify return code"
Verify return code: 0 (ok)
    Verify return code: 0 (ok)

```

## Partition Table

The OTA update requires a partition table that supports OTA functionality. The partition table should define at least
two app partitions for the firmware images and an OTA data partition to manage the OTA process.

### Compile Log

Current Application Size is around 1.4MB, so with 20% margin, the OTA partition size should be around 1.7MB.

`Flash: [=====     ]  45.5% (used 1430241 bytes from 3145728 bytes)`

### Partition Table

There are pre-defined partition https://github.com/espressif/arduino-esp32/blob/master/tools/partitions/default.csv from
ESP32. But, the application partition is too small for current application.

## Partition Table Layout

### partition/4MB-ota.csv

It is used for ESP32 with 4MB flash size. very tight fit for current application size.

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x180000,
app1,     app,  ota_1,   0x190000,0x180000,
spiffs,   data, spiffs,  0x310000,0x5000,
coredump, data, coredump,0x315000,0xEB000,
```

Human-readable format:

| Name     | Type | SubType  | Offset   | Size     | MiB  | Bytes     | Description                               |
|----------|------|----------|----------|----------|------|-----------|-------------------------------------------|
| nvs      | data | nvs      | 0x9000   | 0x5000   | 0.02 | 20,480    | Non-Volatile Storage (WiFi, config)       |
| otadata  | data | ota      | 0xe000   | 0x2000   | 0.01 | 8,192     | OTA metadata (active partition tracking)  |
| app0     | app  | ota_0    | 0x10000  | 0x180000 | 1.50 | 1,572,864 | Primary firmware slot (OTA partition 0)   |
| app1     | app  | ota_1    | 0x190000 | 0x180000 | 1.50 | 1,572,864 | Secondary firmware slot (OTA partition 1) |
| spiffs   | data | spiffs   | 0x310000 | 0x5000   | 0.02 | 20,480    | File system (web interface, certificates) |
| coredump | data | coredump | 0x315000 | 0xEB000  | 0.92 | 962,560   | Core dump storage for crash debugging     |

**Summary:**

- **Total used**: 3.97 MiB (4,157,440 bytes)
- **Current app size**: 1.36 MiB (1,430,241 bytes)
- **Margin per OTA partition**: ~0.14 MiB (142,623 bytes) — **9% headroom**

### partitions/default_16MB.csv

ESP32S3 Plus has 16MB flash size. pre-defined partition is being used

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x640000,
app1,     app,  ota_1,   0x650000,0x640000,
spiffs,   data, spiffs,  0xc90000,0x360000,
coredump, data, coredump,0xFF0000,0x10000,
```

| Name     | Type | SubType  | Offset   | Size     | MiB  | Bytes     | Description                               |
|----------|------|----------|----------|----------|------|-----------|-------------------------------------------|
| nvs      | data | nvs      | 0x9000   | 0x5000   | 0.02 | 20,480    | Non-Volatile Storage (WiFi, config)       |
| otadata  | data | ota      | 0xe000   | 0x2000   | 0.01 | 8,192     | OTA metadata (active partition tracking)  |
| app0     | app  | ota_0    | 0x10000  | 0x640000 | 6.25 | 6,553,600 | Primary firmware slot (OTA partition 0)   |
| app1     | app  | ota_1    | 0x650000 | 0x640000 | 6.25 | 6,553,600 | Secondary firmware slot (OTA partition 1) |
| spiffs   | data | spiffs   | 0xc90000 | 0x360000 | 3.38 | 3,538,944 | File system (web interface, certificates) |
| coredump | data | coredump | 0xFF0000 | 0x10000  | 0.06 | 65,536    | Core dump storage for crash debugging     |

Summary:
Total used: 15.97 MiB (16,740,352 bytes)
Current app size: 1.36 MiB (1,430,241 bytes)
Margin per OTA partition: ~4.89 MiB (5,123,359 bytes) — 78% headroom (very comfortable!)
