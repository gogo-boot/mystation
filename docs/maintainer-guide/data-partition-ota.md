# Data Partition OTA Update

## Status: Not Applicable

The data partition (LittleFS/SPIFFS) has been **removed** from the partition table.

The HTML configuration page is now embedded directly in the firmware binary via a PROGMEM raw string
literal. This means:

- ✅ The config page is automatically updated via OTA along with the firmware
- ✅ No separate `littlefs.bin` or `U_SPIFFS` update is needed
- ✅ Simpler partition layout with more space for firmware OTA slots
- ✅ No filesystem mount failures or corruption issues

## How It Works

1. **Source**: `data/config_my_station.html` (developer-editable HTML)
2. **Build script**: `tools/embed_html.py` converts HTML → `include/config/config_page_html.h`
3. **Firmware**: Serves the HTML from PROGMEM via `CONFIG_PAGE_HTML` constant
4. **OTA**: Standard firmware OTA delivers everything in one binary

## Historical Context

This document previously described a plan to implement `U_SPIFFS` OTA updates for the data partition.
That approach was abandoned in favor of embedding the HTML directly in firmware, which is simpler
and eliminates the need for a separate data partition entirely.

See [OTA Update](./ota-update.md) for the current OTA implementation.
