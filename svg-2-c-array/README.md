# SVG to C Header Conversion

Converts SVG icons into C header files containing bitmap arrays for the e-paper display (AdafruitGFX `PROGMEM` format).

The `svg/` directory is the **single source of truth** for all icon images.

## How It Works

The pipeline converts vector icons into embedded binary data that the ESP32 can render on the e-paper display.

### Step 1: SVG → PNG (Inkscape)

Each SVG is rasterized at multiple fixed sizes (24, 32, 48, 64 pixels).
Inkscape renders with a white background since the e-paper display is monochrome (black/white).

```
svg/wi-0-day-sunny.svg  →  png/24x24/wi-0-day-sunny.png
                        →  png/32x32/wi-0-day-sunny.png
                        →  png/48x48/wi-0-day-sunny.png
                        →  png/64x64/wi-0-day-sunny.png
```

### Step 2: PNG → C Header (`png_to_header.py`)

Each PNG is converted to a C array of bytes stored in `PROGMEM` (flash memory).
The script:
1. Opens the PNG and converts to grayscale
2. Applies a threshold (pixel > 127 = white/1, else black/0)
3. Packs 8 pixels into each byte (MSB first)
4. Writes the byte array as a C header file

The variable name is derived from the filename (hyphens become underscores):

```
png/64x64/wi-0-day-sunny.png  →  lib/bitmap_images/64x64/wi_0_day_sunny_64x64.h
```

The generated header looks like:
```c
// 64 x 64
const unsigned char wi_0_day_sunny_64x64[] PROGMEM = {
  0xff, 0xff, 0xe0, 0x07, ...
};
```

### Step 3: Generate Index (`final_generate_icons_h.py`)

This script scans `lib/bitmap_images/` and generates:

1. **`icons_NxN.h`** (one per size) — include lists for all headers at that size
2. **`icons.h`** — a unified header containing:
   - An `icon_name_t` enum with all icon names
   - A `getBitmap(icon, size)` function that dispatches to the correct array

This allows application code to reference icons by name and size:
```c
const unsigned char* bmp = getBitmap(wi_0_day_sunny, 64);
display.drawBitmap(x, y, bmp, 64, 64, GxEPD_BLACK);
```

## Dependencies

- **Inkscape** — SVG to PNG rasterization (`brew install inkscape`)
- **Python 3** — runs conversion scripts (venv created automatically by Makefile)

## Usage

```sh
make
```

That's it. The Makefile automatically:
1. Creates a Python venv and installs Pillow (first run only)
2. Deletes old PNG cache (prevents stale orphans from deleted SVGs)
3. Rasterizes all SVGs at all sizes via Inkscape
4. Converts all PNGs to C header files
5. Regenerates `icons_NxN.h` and `icons.h`

> **Do NOT run `make -j`** — Inkscape has a bug with parallel conversions.
> https://gitlab.com/inkscape/inkscape/-/issues/4716

## Makefile Targets

| Target | Command | What it does |
|--------|---------|--------------|
| `all` (default) | `make` | Full clean rebuild — the standard command |
| `icons` | `make icons` | Only regenerate `icons.h` + `icons_NxN.h` (no Inkscape, fast) |
| `clean` | `make clean` | Delete PNG cache and venv |

### When to use `make` vs `make icons`

- **`make`** — use whenever SVGs are added, removed, or changed. Always does a full
  clean rebuild so no orphaned files from deleted SVGs remain.
- **`make icons`** — use when you only need to regenerate the index files (e.g. after
  manually deleting a stale header from `lib/bitmap_images/`). Skips Inkscape entirely.

## Adding a New Icon

1. Place the SVG file in `svg/`
2. Run `make`
3. Verify: `pio run`

## File Naming

- SVG filenames may use hyphens: `wi-0-day-sunny.svg`
- The conversion replaces all non-alphanumeric chars with `_`: → `wi_0_day_sunny_64x64.h`
- **Do NOT create duplicates** that differ only by hyphen/underscore (e.g. `wifi-off.svg` and `wifi_off.svg` collide)

## Enum-Only Icons

Some icon names are referenced in code but don't have SVG sources yet (placeholders).
These are listed in `ENUM_ONLY_ICONS` in `final_generate_icons_h.py`. They appear in the
enum so the code compiles, but `getBitmap()` returns `nullptr` for them.

## License

Icons in `svg/` remain licensed under their original agreements:

- **Weather Icons** (`wi-*.svg`) — [SIL OFL 1.1](http://scripts.sil.org/OFL) / [MIT](http://opensource.org/licenses/mit-license.html)
  Source: https://github.com/erikflowers/weather-icons
- **Battery Icons** (`Battery*.svg`) — [Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0.txt)
  Source: https://fonts.google.com/icons
- **WiFi Icons** (`wifi*.svg`) — [MIT](http://opensource.org/licenses/mit-license.html)
  Source: https://github.com/phosphor-icons/homepage
- **Misc Icons** (`refresh.svg`, etc.) — [MIT](http://opensource.org/licenses/mit-license.html)
  Source: https://tabler.io/icons
