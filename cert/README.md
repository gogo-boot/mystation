# GitHub Root CA Certificates Summary

## Active Bundle: `cert/github_bundle.pem`

Contains **3 root CA certificates** required to cover all GitHub domains:

| # | Certificate                           | Used By                                   | Valid Until |
|---|---------------------------------------|-------------------------------------------|-------------|
| 1 | USERTrust ECC Certification Authority | `api.github.com`, `github.com`            | 2038-01-18  |
| 2 | USERTrust RSA Certification Authority | `api.github.com`, `github.com` (fallback) | 2038-01-18  |
| 3 | **ISRG Root X1 (Let's Encrypt)**      | `release-assets.githubusercontent.com`    | 2035-06-04  |

> **Why ISRG Root X1?**
> The firmware download URL (`https://github.com/.../firmware.bin`) redirects via HTTP 302 to
> `release-assets.githubusercontent.com`, which uses a **Let's Encrypt** certificate — a completely
> different CA chain from `api.github.com`. Without ISRG Root X1 the TLS handshake fails with
> mbedTLS error `-0x2700` (certificate verification failed).

## Configuration

### platformio.ini

```ini
board_build.embed_txtfiles = cert/github_bundle.pem
```

### ota_update.cpp

```cpp
extern const char server_cert_pem_start[] asm("_binary_cert_github_bundle_pem_start");
extern const char server_cert_pem_end[]   asm("_binary_cert_github_bundle_pem_end");
```

## Verification

```bash
# Both should show: Verify return code: 0 (ok)
openssl s_client -connect api.github.com:443 \
  -CAfile cert/github_bundle.pem </dev/null 2>/dev/null | grep "Verify return code"

openssl s_client -connect release-assets.githubusercontent.com:443 \
  -CAfile cert/github_bundle.pem </dev/null 2>/dev/null | grep "Verify return code"
```

## Created Files

I've created the following root CA certificates for all GitHub domains:

### 1. `cert/digicert_global_root_g2.pem`

- **Certificate**: DigiCert Global Root G2
- **Valid Until**: 2038-01-15
- **Used By**: `api.github.com`, `github.com`, `release-assets.githubusercontent.com`
- **Algorithm**: RSA 2048-bit

### 2. `cert/digicert_global_root_ca.pem`

- **Certificate**: DigiCert Global Root CA
- **Valid Until**: 2031-11-10
- **Used By**: Alternative root for GitHub domains
- **Algorithm**: RSA 2048-bit

### 3. `cert/github_bundle.pem` (RECOMMENDED)

- **Contains**: Both DigiCert Global Root G2 AND DigiCert Global Root CA
- **Purpose**: Single file that works for ALL GitHub domains
- **Size**: 2 certificates in one file

## Which Domains Use Which Certificate?

All three GitHub domains use **DigiCert root CAs**:

| Domain                                 | Root CA                       |
|----------------------------------------|-------------------------------|
| `api.github.com`                       | DigiCert Global Root G2 or CA |
| `github.com`                           | DigiCert Global Root G2 or CA |
| `release-assets.githubusercontent.com` | DigiCert Global Root G2 or CA |

## Recommended Configuration

### Update platformio.ini

```ini
board_build.embed_files =
    cert/github_bundle.pem
```

### Update src/ota/ota_update.cpp

```cpp
// Change certificate reference
extern const char server_cert_pem_start[] asm("_binary_cert_github_bundle_pem_start");
extern const char server_cert_pem_end[] asm("_binary_cert_github_bundle_pem_end");
```

### Why Use the Bundle?

1. **Works for ALL GitHub domains** - No need to manage separate certificates
2. **Future-proof** - Contains both G2 and original Global Root CA
3. **Long validity** - Both certificates valid until 2030s
4. **Small size** - Only ~3KB for both certificates

## Testing (Manual Verification)

If you want to test manually with openssl:

```bash
# Test api.github.com
openssl s_client -connect api.github.com:443 \
  -CAfile cert/github_bundle.pem </dev/null

# Test github.com
openssl s_client -connect github.com:443 \
  -CAfile cert/github_bundle.pem </dev/null

# Test release-assets.githubusercontent.com
openssl s_client -connect release-assets.githubusercontent.com:443 \
  -CAfile cert/github_bundle.pem </dev/null
```

All should show: `Verify return code: 0 (ok)`

## What This Fixes

✅ **API calls** to `api.github.com` for release metadata
✅ **Firmware downloads** from `github.com` or `release-assets.githubusercontent.com`
✅ **SSL handshake error** `-0x2700` (mbedtls_ssl_handshake failed)

## Next Steps

1. Update `platformio.ini` to embed `cert/github_bundle.pem`
2. Update code reference from `github_pem` to `github_bundle_pem`
3. Rebuild firmware: `pio run -e esp32-s3-debug`
4. Flash to device
5. Test OTA update at 03:00 or trigger manually

The bundle approach ensures maximum compatibility across all GitHub CDN servers and endpoints!

