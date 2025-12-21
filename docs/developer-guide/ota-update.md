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

To download the firmware binary from GitHub securely over HTTPS, the device needs to trust the CA certificates used by
The `firmware.bin` file is hosted on GitHub's release assets domain, which uses SSL/TLS certificates issued by trusted
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

In certificates chin, the Root CA is not included in the server certificate chain sent by the server.
You need to find out the `Issuer`

```bash
# Get ALL certificates in the chain
openssl s_client -showcerts -connect github.com:443 </dev/null 2>/dev/null | \
  awk '/BEGIN CERTIFICATE/,/END CERTIFICATE/' > cert/github_chain.pem
# Issuer: C=US, O=DigiCert Inc, OU=www.digicert.com, CN=DigiCert Global Root G2
# The CN=DigiCert Global Root G2 tells you which root CA to download.

```

### Download Root CA Certificates

```bash
curl -s https://cacerts.digicert.com/DigiCertGlobalRootG2.crt.pem -o digicert_g2.pem && echo "Downloaded DigiCert Global Root G2"                                                                                                       16:12:47
curl -s https://cacerts.digicert.com/DigiCertGlobalRootCA.crt.pem -o digicert_global_root.pem && echo "Downloaded DigiCert Global Root CA"
cat digicert_g2.pem digicert_global_root.pem > cert/github_bundle.pem && echo "Created bundle with both DigiCert root CAs"
```

### Pem file verification

Test if it can verify the server certificates using the downloaded CA certificates:

View Certificates info:

```bash
openssl x509 -in cert/github_root_ca.pem -noout -issuer -subject
```

Test connection using the CA file:

```bash
openssl s_client -connect api.github.com:443 -CAfile cert/github_bundle.pem </dev/null 2>&1 | grep "Verify return code"

openssl s_client -connect release-assets.githubusercontent.com:443 -CAfile cert/github_root_browser.pem </dev/null 2>&1 | grep "Verify return code"
Verify return code: 0 (ok)
    Verify return code: 0 (ok)

```
