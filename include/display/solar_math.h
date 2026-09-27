#pragma once
// =============================================================================
// Solar graph math — pure, dependency-free helpers (no display/Arduino deps).
// Extracted so the numeric logic can be unit-tested in the native environment.
// =============================================================================
#include "api/dwd_weather_api.h"

namespace SolarMath {

// Round a peak W/m² value up to a clean axis ceiling: nearest 100 above the
// peak, with a 100 floor so near-dark days still get a sane axis (and the
// curve never divides by zero).
float calculateSolarAxisMax(float peakRadiation);

// Find the peak radiation among available (non-negative) hourly points and the
// count of valid points. Points with solarRadiation < 0 are "unavailable" and
// excluded. Returns the peak (0 if none); validCount is written via out-param.
float findSolarPeak(const WeatherHourlyForecast hourlyData[], int count, int& validCount);

// True if there is enough valid solar data to draw a meaningful curve
// (at least 2 valid points and a positive peak).
bool hasValidSolarData(const WeatherHourlyForecast hourlyData[], int count);

// Normalize a single radiation value to a 0..100 percentage of axisMax.
// Clamps to [0,100]. Returns 0 for unavailable (negative) input.
float normalizeToPercent(float radiation, float axisMax);

} // namespace SolarMath
