# SVG to C Header Conversion

Converts SVG icons into C header files containing bitmap arrays for the e-paper display (AdafruitGFX `PROGMEM` format).

The `svg/` directory is the **single source of truth** for all icon images.

## Pipeline

```
svg/*.svg  →  png/<size>/*.png  →  lib/bitmap_images/<size>x<size>/*.h
                (Inkscape)              (png_to_header.py)

                                  →  lib/bitmap_images/icons_NxN.h
                                  →  lib/bitmap_images/icons.h
                                       (final_generate_icons_h.py)
```

## Dependencies

- **Inkscape** — SVG to PNG rasterization (`brew install inkscape`)
- **Python 3** with **Pillow** — PNG to C-array conversion

## Makefile Targets

| Target | Command | When to Use |
|--------|---------|-------------|
| `all` (default) | `make` | Added/removed/changed SVGs — clean rebuild from scratch |
| `headers-<N>` | `make headers-64` | Generate only one size (e.g. testing a new icon at 64px) |
| `icons` | `make icons` | Regenerate `icons_NxN.h` + `icons.h` without re-rasterizing PNGs |
| `clean` | `make clean` | Delete PNG cache and venv completely |

### When to use `make` vs scripts directly

- **`make`** — the standard command. Always does a clean rebuild: removes stale PNGs,
  re-rasterizes all SVGs, regenerates all headers, and rebuilds `icons.h`.
  This ensures no orphaned files from deleted SVGs remain.
- **`make icons`** — use after manually deleting stale header files from `lib/bitmap_images/`,
  or after adding a manually-created header (not from SVG). Skips Inkscape entirely.
- **`python3 final_generate_icons_h.py`** — same as `make icons`, use when you don't have
  `make` available or want to run it from a different directory.
- **`bash svg_to_headers.sh 64`** — use to test a single size before running the full pipeline.

## Usage

```sh
make
```

That's it. The Makefile automatically creates a Python venv, installs Pillow, runs all
conversions, and generates `icons.h`. Inkscape must be installed separately (`brew install inkscape`).

> **Do NOT run `make -j`** — Inkscape has a bug with parallel conversions.
> https://gitlab.com/inkscape/inkscape/-/issues/4716

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
