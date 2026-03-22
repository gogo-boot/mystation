# Certificate Bundle for OTA Updates

The OTA process makes **two separate TLS connections** to two different hosts that use **different root CAs**:

| Step | Host                                   | Purpose                                       | Root CA                          |
|------|----------------------------------------|-----------------------------------------------|----------------------------------|
| 1    | `api.github.com`                       | Fetch release metadata (tag name, asset URLs) | USERTrust ECC                    |
| 2    | `release-assets.githubusercontent.com` | Download `firmware.bin`                       | **ISRG Root X1 (Let's Encrypt)** |

Step 1 uses `HTTPClient`. Step 2 uses `esp_https_ota`.
Both share the **same embedded certificate** (`server_cert_pem_start`) configured in `ota_update.cpp`.
Therefore the single embedded cert file must cover **both root CAs**.

:::warning Why two CAs?
The firmware download URL `https://github.com/.../firmware.bin` returns an HTTP **302 redirect**
to `release-assets.githubusercontent.com`. If only the USERTrust cert is embedded, Step 1 passes
but Step 2 fails with mbedTLS error **`-0x2700`** (certificate verification failed).
:::

---

## Active bundle: `cert/github_bundle.pem`

Contains **3 root CA certificates** (as of March 2026):

| # | CN                                    | Used by                                   | Valid until | SHA1 Fingerprint                                              |
|---|---------------------------------------|-------------------------------------------|-------------|---------------------------------------------------------------|
| 1 | USERTrust ECC Certification Authority | `api.github.com`, `github.com`            | 2038-01-18  | `D1:CB:CA:5D:B2:D5:2A:7F:69:3B:67:4D:E5:F0:5A:1D:0C:95:7D:F0` |
| 2 | USERTrust RSA Certification Authority | `api.github.com`, `github.com` (fallback) | 2038-01-18  | `2B:8F:1B:57:33:0D:BB:A2:D0:7A:6C:51:F7:0E:E9:0D:DA:B9:AD:8E` |
| 3 | ISRG Root X1                          | `release-assets.githubusercontent.com`    | 2035-06-04  | `CA:BD:2A:79:A1:07:6A:31:F2:1D:25:36:35:CB:03:9D:43:29:A5:E8` |

---

## Rebuilding `github_bundle.pem`

Run through these steps whenever a certificate expires or GitHub switches CA provider.

### Step 1 — Inspect the current certificate chains

Check what root CA each host uses **right now**:

```bash
for host in api.github.com github.com release-assets.githubusercontent.com; do
  echo "=== $host ==="
  echo | openssl s_client -connect $host:443 2>/dev/null | grep -E "( s:| i:)"
  echo ""
done
```

Each line shows `s:` (subject) and `i:` (issuer). The **last `i:` in the chain** is the root CA you need.

<details>
<summary>Example output (March 2026)</summary>

```
=== api.github.com ===
 0 s:CN=*.github.com
    i:C=GB, O=Sectigo Limited, CN=Sectigo Public Server Authentication CA DV E36
 1 s:C=GB, O=Sectigo Limited, CN=Sectigo Public Server Authentication CA DV E36
    i:C=GB, O=Sectigo Limited, CN=Sectigo Public Server Authentication Root E46
 2 s:C=GB, O=Sectigo Limited, CN=Sectigo Public Server Authentication Root E46
    i:C=US, ..., CN=USERTrust ECC Certification Authority   ← root CA needed

=== github.com ===
 0 s:CN=github.com
    i:C=GB, O=Sectigo Limited, CN=Sectigo Public Server Authentication CA DV E36
 ...
    i:C=US, ..., CN=USERTrust ECC Certification Authority   ← same root CA

=== release-assets.githubusercontent.com ===
 0 s:CN=*.github.io
    i:C=US, O=Let's Encrypt, CN=R12
 1 s:C=US, O=Let's Encrypt, CN=R12
    i:C=US, O=Internet Security Research Group, CN=ISRG Root X1   ← root CA needed
```

</details>

### Step 2 — Extract each root CA from the macOS system keychain

```bash
# USERTrust ECC  (covers api.github.com / github.com)
security find-certificate -c "USERTrust ECC Certification Authority" \
  -p /System/Library/Keychains/SystemRootCertificates.keychain \
  > cert/usertrust_ecc.pem

# USERTrust RSA  (RSA fallback for the same hosts)
security find-certificate -c "USERTrust RSA Certification Authority" \
  -p /System/Library/Keychains/SystemRootCertificates.keychain \
  > cert/usertrust_rsa.pem

# ISRG Root X1  (covers release-assets.githubusercontent.com via Let's Encrypt)
security find-certificate -c "ISRG Root X1" \
  -p /System/Library/Keychains/SystemRootCertificates.keychain \
  > cert/isrg_root_x1.pem
```

:::note Alternative source for ISRG Root X1
If it is not in your keychain:

```bash
curl -sO https://letsencrypt.org/certs/isrgrootx1.pem
# Verify fingerprint before using:
openssl x509 -in isrgrootx1.pem -noout -fingerprint
# Expected: CA:BD:2A:79:A1:07:6A:31:F2:1D:25:36:35:CB:03:9D:43:29:A5:E8
```

:::

### Step 3 — Verify each cert before bundling

```bash
for f in cert/usertrust_ecc.pem cert/usertrust_rsa.pem cert/isrg_root_x1.pem; do
  echo "=== $f ==="
  openssl x509 -in $f -noout -subject -dates -fingerprint
done
```

### Step 4 — Build the bundle

```bash
cat cert/usertrust_ecc.pem \
    cert/usertrust_rsa.pem \
    cert/isrg_root_x1.pem \
    > cert/github_bundle.pem
```

### Step 5 — Verify the bundle validates both hosts

```bash
echo "--- api.github.com ---"
echo | openssl s_client -connect api.github.com:443 \
  -CAfile cert/github_bundle.pem 2>/dev/null | grep "Verify return code"

echo "--- release-assets.githubusercontent.com ---"
echo | openssl s_client -connect release-assets.githubusercontent.com:443 \
  -CAfile cert/github_bundle.pem 2>/dev/null | grep "Verify return code"
```

Both must show: `Verify return code: 0 (ok)`

### Step 6 — Rebuild and flash firmware

```bash
pio run -e esp32-s3-e1001-ota-test --target upload
```

---

## Inspecting a released firmware

These commands let you verify what is published on GitHub **before** flashing — useful for
diagnosing OTA failures (wrong partition table, wrong build environment, firmware too large, etc.).

Replace `v0.4.0` with whatever release you want to inspect.

### List all release assets and their sizes

```bash
curl -s https://api.github.com/repos/gogo-boot/mystation/releases/latest \
  | python3 -c "
import sys, json
r = json.load(sys.stdin)
print('Tag:', r['tag_name'])
print('Published:', r['published_at'])
print()
for a in r['assets']:
    print('  {:50s} {:>10,} bytes'.format(a['name'], a['size']))
"
```

<details>
<summary>Example output (v0.4.0)</summary>

```
Tag: v0.4.0
Published: 2025-12-21T21:29:30Z

  bootloader.bin                                         15,104 bytes
  build_info.txt                                            202 bytes
  config_my_station.html                                 40,225 bytes
  firmware.bin                                        1,329,280 bytes
  partitions.bin                                          3,072 bytes
  VERSION.txt                                               118 bytes
```

</details>

:::tip
Cross-check `firmware.bin` size against the `app0`/`app1` OTA slot size from the partition table below.
If `firmware.bin` is larger than the OTA slot, `esp_https_ota` will refuse to write it.
:::

### Show build environment and PlatformIO version

```bash
curl -sL https://github.com/gogo-boot/mystation/releases/download/v0.4.0/build_info.txt
```

<details>
<summary>Example output (v0.4.0)</summary>

```
Build Information
==================
Commit: c690ed057111b055601d7ee228fb9331bef011a2
Branch: v0.4.0
Environment: esp32-s3-production
Build Date: Sun Dec 21 21:30:56 UTC 2025
PlatformIO Version: 6.1.18
```

</details>

:::warning Environment mismatch
The **Environment** field tells you which `platformio.ini` env produced the binary.
The partition table flashed on your device must match this environment.
A mismatch is a common cause of `Failed to establish HTTP connection` errors from `esp_https_ota`.
:::

### Decode the partition table

```bash
python3 - << 'EOF'
import struct, urllib.request

url = "https://github.com/gogo-boot/mystation/releases/download/v0.4.0/partitions.bin"
data = urllib.request.urlopen(url).read()

print("Partition table:")
print("{:<16} {:<8} {:<10} {:<12} {:<12} {:<10}".format(
    'Name', 'Type', 'SubType', 'Offset', 'Size', 'Size(KB)'))
print("-" * 70)
for i in range(0, len(data), 32):
    entry = data[i:i+32]
    if len(entry) < 32 or entry[0] != 0xAA or entry[1] != 0x50:
        break
    ptype   = entry[2]
    subtype = entry[3]
    offset  = struct.unpack_from('<I', entry, 4)[0]
    size    = struct.unpack_from('<I', entry, 8)[0]
    name    = entry[12:28].rstrip(b'\x00').decode('ascii', errors='replace')
    print("{:<16} {:<8} {:<10} {:<12} {:<12} {:<10}".format(
        name, ptype, subtype, hex(offset), hex(size), size // 1024))
EOF
```

<details>
<summary>Example output (v0.4.0)</summary>

```
Partition table:
Name             Type     SubType    Offset       Size         Size(KB)
----------------------------------------------------------------------
nvs              1        2          0x9000       0x5000       20
otadata          1        0          0xe000       0x2000       8
app0             0        16         0x10000      0x640000     6400
app1             0        17         0x650000     0x640000     6400
spiffs           1        130        0xc90000     0x360000     3456
coredump         1        3          0xff0000     0x10000      64
```

`app0` and `app1` are the two OTA slots — **6400 KB each**.
`firmware.bin` at **1298 KB** fits with ~78% headroom.

</details>

---

## Code reference

### `platformio.ini`

```ini
board_build.embed_txtfiles = cert/github_bundle.pem
```

### `src/ota/ota_update.cpp`

```cpp
extern const char server_cert_pem_start[] asm("_binary_cert_github_bundle_pem_start");
extern const char server_cert_pem_end[]   asm("_binary_cert_github_bundle_pem_end");
```

:::note Symbol name derivation
PlatformIO derives the `asm` symbol name from the file path: slashes and dots → underscores,
prefixed with `_binary_`, suffixed with `_start` / `_end`.

`cert/github_bundle.pem` → `_binary_cert_github_bundle_pem_start`
:::

---

## When to update the bundle

| Trigger                                                                           | Action                                                    |
|-----------------------------------------------------------------------------------|-----------------------------------------------------------|
| A cert in the bundle is nearing expiry                                            | Redo Steps 1–6                                            |
| GitHub switches CA provider (happened Dec 2025: DigiCert → Sectigo/Let's Encrypt) | Redo Steps 1–6                                            |
| OTA fails with `-0x2700` in the serial log                                        | Run Step 1 to detect CA change, then redo Steps 2–6       |
| OTA fails with `Failed to establish HTTP connection` after ~10 s timeout          | Same as above — likely a CA mismatch on the download host |

