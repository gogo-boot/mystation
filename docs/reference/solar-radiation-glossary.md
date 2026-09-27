# Solar Radiation Feature — Glossary

This reference records the agreed terminology for the two day-browse graph views
(weather and solar). It exists so that code, on-screen labels, and documentation stay
consistent. See issue #439 for the discussion that produced these terms.

> This is a terminology reference only. The narrative developer-guide documentation for
> the solar radiation feature is maintained separately (see #440).

## English code & architecture terms

| Concept | Agreed term | Notes |
|---------|-------------|-------|
| Umbrella term for stepping through forecast days | **Day browse** | Both weather and solar browse are day-browse variants. |
| Existing per-day temperature + rain graph view | **Weather browse** | The weather variant of day browse. |
| New per-day solar radiation graph view | **Solar browse** | The solar variant of day browse. |
| RTC state selecting which variant is active | **Browse context** | `BrowseContext { WEATHER, SOLAR }` — new `RTC_DATA_ATTR` field `browseContext`. |
| RTC field: which day is shown | `selectedForecastDay` | Unchanged. `0` = today, `1`–`6` = future days. |
| RTC field: how many forecast days are available | `availableForecastDays` | Unchanged. Depends on the weather model (1–7). |
| API variable | `shortwave_radiation` | Open-Meteo hourly variable, unit W/m². |
| Struct field | `solarRadiation` | Per-hour value carried in `WeatherHourlyForecast` / `DayBrowsePoint`. |

## German on-screen label

The solar view title uses **"Sonnenstrom"** ("sun electricity") — a warm, product-marketing
term chosen for a Balkonkraftwerk/PV audience: energy-framed and relatable without falsely
implying the graph reads an actual inverter (it shows the solar *resource*, `shortwave_radiation`).
The day's total energy is appended as `Sonnenstrom - X.X kWh/m2`.

Rejected alternatives: `Sonneneinstrahlung` (accurate but clinical/long), `PV-Ertrag` (strong
keyword recognition but implies measured yield), `Balkonkraftwerk` (too narrow — excludes
rooftop/ground PV), bare `Sonne` (ambiguous with weather).

Latin-1 safe for the u8g2 renderer; fits the weather-full title area (800 px wide).

## Button-behavior phrasing (for consistent docs)

- "Button 2 enters/continues **weather browse**; Button 3 enters/continues **solar browse**."
- "Within a browse context, Button 2 = next day (+1), Button 3 = previous day (-1)."
- "Button 1 = back to weather today (resets browse context to `WEATHER`)."

## Rationale for keeping "day browse" as the umbrella

The existing feature was loosely called "day browse" / "weather-only day browsing" in the code
and docs. Rather than rename it (which would churn `button-system.md`, `display-system.md`, and
button-routing code), "day browse" is kept as the umbrella term and the two variants —
**weather browse** and **solar browse** — are added alongside it. This minimizes changes to
existing code and documentation.
