# Troubleshooting

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
