#include "display/solar_math.h"
#include <math.h>

namespace SolarMath {

float calculateSolarAxisMax(float peakRadiation) {
    if (peakRadiation < 100.0f) return 100.0f;
    return ceilf(peakRadiation / 100.0f) * 100.0f;
}

float findSolarPeak(const WeatherHourlyForecast hourlyData[], int count, int& validCount) {
    float peak = 0.0f;
    validCount = 0;
    for (int i = 0; i < count; i++) {
        if (hourlyData[i].solarRadiation >= 0.0f) {
            validCount++;
            if (hourlyData[i].solarRadiation > peak) {
                peak = hourlyData[i].solarRadiation;
            }
        }
    }
    return peak;
}

bool hasValidSolarData(const WeatherHourlyForecast hourlyData[], int count) {
    if (count < 2) return false;
    int validCount = 0;
    float peak = findSolarPeak(hourlyData, count, validCount);
    return (validCount >= 2 && peak > 0.0f);
}

float normalizeToPercent(float radiation, float axisMax) {
    if (radiation < 0.0f || axisMax <= 0.0f) return 0.0f;
    float pct = (radiation / axisMax) * 100.0f;
    if (pct < 0.0f) pct = 0.0f;
    if (pct > 100.0f) pct = 100.0f;
    return pct;
}

} // namespace SolarMath
