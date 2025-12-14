# Project Run Book

## To Run Docusaurus Locally

It is useful to run Docusaurus locally to preview the changes before pushing to remote repository.

```bash
cd website
npm install
npm run build
npm run serve
```

## Platform IO Commands

List Connected Devices

```bash
pio device list
```

Monitor Serial Output

```bash
pio device monitor
```

Unit Test

```bash
pio test -e native -v
```

Build Project with default environment

```bash
pio run
```

##

## Troubleshooting

### Board Not Detected

Make sure the USB cable is properly connected and is a data-capable cable (some cables are power-only). Check that the
correct drivers for your board are installed.

If the Board is in deep sleep mode, press the reset button to wake it up before uploading.

If the cable is already connected, you press and hold the BOOT key then press the Reset key once, you can
enter `BootLoader Mode`

When it somehow still gets stuck in deep sleep the device won't be detected. while push and holding the boot button and
connect the cable again. then it will be in `Bootloader Mode`

```bash
# List available devices
pio device list

# Manual port specification
pio run --target upload --upload-port /dev/ttyUSB0
```

### Filesystem Upload Fails

```bash
# Erase flash completely
pio run --target erase

# Re-upload firmware and filesystem
pio run --target upload
pio run --target uploadfs
```

## Related Documentation

- [Development Setup](development-setup.md) - Initial setup and configuration
- [GitHub Actions](github-actions.md) - CI/CD workflows and automated builds
- [Testing](testing.md) - Testing procedures and debugging
- [OTA System](ota-system.md) - Firmware update procedures
- [User Troubleshooting Guide](../user-guide/troubleshooting.md) - User-facing troubleshooting

