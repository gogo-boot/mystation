# Icon Design

MyStation renders all of its on-screen symbols — weather conditions, battery level, WiFi
signal, refresh, sunrise/sunset — as monochrome bitmaps on the e-paper display. This page
explains where the icons come from, the design constraints they must satisfy, and how a
vector source becomes an embedded C array the firmware can draw.

> The build tool that performs the conversion is documented in
> [`svg-2-c-array/README.md`](https://github.com/gogo-boot/mystation/blob/main/svg-2-c-array/README.md).
> This page focuses on the **design** side: sources, principles, and licensing. The tool
> README is the reference for the exact commands and script internals.

---

## Design Principles

The display is a **7.5″ black-and-white e-paper panel (800×480, no gray levels)**. Icons must
read cleanly at small sizes with only two colors, so the icon set follows a few rules:

| Principle | Why it matters on e-paper |
|-----------|---------------------------|
| **Outline / line style, ~2px stroke** | Thin, evenly-weighted strokes stay crisp after 1-bit thresholding. Filled or gradient icons turn into muddy blobs. |
| **Simple, geometric shapes** | The rasterizer thresholds each pixel to pure black or white — fine detail below ~2px is lost. |
| **Designed on a square grid (24×24 base)** | Icons scale predictably to the fixed render sizes (24 / 32 / 48 / 64 px) without drifting off-center. |
| **High contrast, no anti-alias reliance** | There is no gray to soften edges; shapes must work as hard black-on-white. |
| **Consistent visual weight across a set** | Weather, battery, and WiFi icons sit next to each other — mismatched stroke weights look broken. |

These constraints are exactly what modern **outline icon libraries** are built for, which is
why MyStation sources most of its UI icons from two of them (see below).

---

## Icon Sources

The `svg-2-c-array/svg/` directory is the **single source of truth** for every icon. All
sources are **open-source and free** (including for commercial use). The set is drawn from
several curated libraries:

| Icon group | Files | Source | License |
|------------|-------|--------|---------|
| Weather conditions | `wi-*.svg` | [Weather Icons](https://github.com/erikflowers/weather-icons) | SIL OFL 1.1 / MIT |
| Battery levels | `Battery-*.svg` | [Material Symbols](https://fonts.google.com/icons) | Apache 2.0 |
| WiFi signal | `wifi*.svg` | [Phosphor Icons](https://github.com/phosphor-icons/homepage) | MIT |
| UI / misc (refresh, etc.) | `refresh.svg`, … | **Lucide** / **Tabler Icons** | ISC / MIT |

### Lucide

[Lucide](https://lucide.dev/icons/) ([GitHub](https://github.com/lucide-icons/lucide)) is a
community-maintained, open-source icon toolkit (a fork of Feather Icons) with 1600+ vector
icons. It is designed as clean, uniform **outline** icons on a 24×24 grid — a natural fit for
the e-paper design constraints above.

- **License:** [ISC](https://github.com/lucide-icons/lucide/blob/main/LICENSE) — free for
  personal and commercial use.

### Tabler Icons

[Tabler Icons](https://tabler.io/icons)
([GitHub](https://github.com/tabler/tabler-icons)) is a set of 6000+ free, high-quality SVG
icons, each drawn on a **24×24 grid with a 2px stroke**. Because the whole set shares one
stroke weight and grid, icons picked from it stay visually consistent with the rest of the UI.

- **License:** [MIT](https://github.com/tabler/tabler-icons/blob/main/LICENSE) — free for
  personal and commercial use.

> **Picking a new UI icon:** prefer Lucide or Tabler for generic UI glyphs (arrows, refresh,
> settings, info, etc.). Both are outline sets on a 24×24 grid with a ~2px stroke, so they mix
> cleanly. Download the raw SVG and drop it into `svg-2-c-array/svg/`.

---

## From SVG to Display

The firmware cannot render SVG directly — the ESP32 draws pre-rasterized 1-bit bitmaps from
flash (`PROGMEM`). A build-time pipeline converts each SVG source into C arrays at the fixed
render sizes.

```
svg-2-c-array/svg/refresh.svg
        │  (1) Inkscape rasterize @ 24/32/48/64 px, white background
        ▼
svg-2-c-array/png/64x64/refresh.png
        │  (2) png_to_header.py — grayscale, threshold >127, pack 8 px/byte (MSB first)
        ▼
lib/bitmap_images/64x64/refresh_64x64.h   →  const unsigned char refresh_64x64[] PROGMEM = { ... };
        │  (3) final_generate_icons_h.py — build icon_name_t enum + getBitmap(icon, size)
        ▼
lib/bitmap_images/icons.h                 →  application code: getBitmap(refresh, 64)
```

The application then draws by name and size:

```cpp
const unsigned char* bmp = getBitmap(refresh, 64);
display.drawBitmap(x, y, bmp, 64, 64, GxEPD_BLACK);
```

### Why bitmaps instead of SVG at runtime?

- **No SVG renderer on-device** — shipping one would cost flash and CPU the deep-sleep budget
  can't spare.
- **Deterministic 1-bit output** — thresholding happens once at build time, so what you see in
  the PNG cache is exactly what the panel shows.
- **Fixed sizes** — only 24/32/48/64 px are ever needed on the layout, so pre-rendering those
  four is cheaper than scaling at runtime.

---

## Adding or Changing an Icon

1. Add (or replace) the SVG in `svg-2-c-array/svg/` — prefer an **outline** icon on a 24×24
   grid (Lucide or Tabler for UI glyphs). Match the existing stroke weight.
2. Regenerate the headers:
   ```bash
   cd svg-2-c-array && make
   ```
   `make` does a full clean rebuild (Inkscape → PNG → C headers → `icons.h`). Do **not** use
   `make -j` — Inkscape has a
   [parallel-conversion bug](https://gitlab.com/inkscape/inkscape/-/issues/4716).
3. Verify the firmware still builds:
   ```bash
   pio run
   ```
4. Check the rendered PNG cache (`svg-2-c-array/png/`) to confirm the icon thresholds cleanly
   at small sizes — thin details that vanish at 24 px need a simpler icon.

> **Naming:** hyphens in SVG filenames become underscores in the generated variable
> (`wi-0-day-sunny.svg` → `wi_0_day_sunny_64x64`). Never create two files that differ only by
> hyphen vs. underscore — they collide.

---

## Licensing Summary

All icon sources are open-source and free for commercial use. When adding icons from a new
library, record its license in the **License** section of
[`svg-2-c-array/README.md`](https://github.com/gogo-boot/mystation/blob/main/svg-2-c-array/README.md)
so attribution stays complete.

| Library | License | Free for commercial use |
|---------|---------|-------------------------|
| Weather Icons | SIL OFL 1.1 / MIT | ✅ |
| Material Symbols | Apache 2.0 | ✅ |
| Phosphor Icons | MIT | ✅ |
| Lucide | ISC | ✅ |
| Tabler Icons | MIT | ✅ |

---

## Related

- [`svg-2-c-array/README.md`](https://github.com/gogo-boot/mystation/blob/main/svg-2-c-array/README.md) — the conversion tool (commands, script internals, enum-only icons)
- [Display System](display-system.md) — how icons are placed in the e-paper layouts
- [Battery Management](battery-management.md) — battery icon levels
