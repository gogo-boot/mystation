#include <unity.h>
#include "display/solar_math.h"

// Build a WeatherHourlyForecast with only solarRadiation set (other fields
// irrelevant to SolarMath). solarRadiation < 0 means "unavailable".
static WeatherHourlyForecast makePoint(float solar) {
    WeatherHourlyForecast p = {};
    p.solarRadiation = solar;
    return p;
}

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------------------
// calculateSolarAxisMax — clean ceiling rounding
// ---------------------------------------------------------------------------
void test_axis_max_rounds_up_to_next_100(void) {
    TEST_ASSERT_EQUAL_FLOAT(600.0f, SolarMath::calculateSolarAxisMax(585.0f));
    TEST_ASSERT_EQUAL_FLOAT(700.0f, SolarMath::calculateSolarAxisMax(601.0f));
    TEST_ASSERT_EQUAL_FLOAT(500.0f, SolarMath::calculateSolarAxisMax(473.0f));
}

void test_axis_max_exact_multiple_stays(void) {
    // Exact multiples of 100 should not jump to the next bucket.
    TEST_ASSERT_EQUAL_FLOAT(600.0f, SolarMath::calculateSolarAxisMax(600.0f));
    TEST_ASSERT_EQUAL_FLOAT(1000.0f, SolarMath::calculateSolarAxisMax(1000.0f));
}

void test_axis_max_floor_for_near_dark(void) {
    // Near-dark days floor at 100 so the axis is never degenerate / divide-by-zero.
    TEST_ASSERT_EQUAL_FLOAT(100.0f, SolarMath::calculateSolarAxisMax(40.0f));
    TEST_ASSERT_EQUAL_FLOAT(100.0f, SolarMath::calculateSolarAxisMax(0.0f));
    TEST_ASSERT_EQUAL_FLOAT(100.0f, SolarMath::calculateSolarAxisMax(99.9f));
}

void test_axis_max_high_summer(void) {
    TEST_ASSERT_EQUAL_FLOAT(900.0f, SolarMath::calculateSolarAxisMax(900.0f));
    TEST_ASSERT_EQUAL_FLOAT(1000.0f, SolarMath::calculateSolarAxisMax(950.0f));
}

// ---------------------------------------------------------------------------
// findSolarPeak — peak + valid count, skipping unavailable
// ---------------------------------------------------------------------------
void test_find_peak_basic(void) {
    WeatherHourlyForecast pts[] = {
        makePoint(0.0f), makePoint(120.0f), makePoint(585.0f), makePoint(300.0f)
    };
    int validCount = 0;
    float peak = SolarMath::findSolarPeak(pts, 4, validCount);
    TEST_ASSERT_EQUAL_FLOAT(585.0f, peak);
    TEST_ASSERT_EQUAL_INT(4, validCount);
}

void test_find_peak_skips_unavailable(void) {
    // Negative values are "unavailable" and must be excluded from peak + count.
    WeatherHourlyForecast pts[] = {
        makePoint(-1.0f), makePoint(200.0f), makePoint(-1.0f), makePoint(450.0f)
    };
    int validCount = 0;
    float peak = SolarMath::findSolarPeak(pts, 4, validCount);
    TEST_ASSERT_EQUAL_FLOAT(450.0f, peak);
    TEST_ASSERT_EQUAL_INT(2, validCount);
}

void test_find_peak_all_unavailable(void) {
    WeatherHourlyForecast pts[] = { makePoint(-1.0f), makePoint(-1.0f) };
    int validCount = 0;
    float peak = SolarMath::findSolarPeak(pts, 2, validCount);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, peak);
    TEST_ASSERT_EQUAL_INT(0, validCount);
}

void test_find_peak_all_zero_night(void) {
    // A fully dark day: all zeros are "valid" (>=0) but peak is 0.
    WeatherHourlyForecast pts[] = { makePoint(0.0f), makePoint(0.0f), makePoint(0.0f) };
    int validCount = 0;
    float peak = SolarMath::findSolarPeak(pts, 3, validCount);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, peak);
    TEST_ASSERT_EQUAL_INT(3, validCount);
}

// ---------------------------------------------------------------------------
// hasValidSolarData — gate for drawing vs fallback
// ---------------------------------------------------------------------------
void test_has_valid_data_true(void) {
    WeatherHourlyForecast pts[] = { makePoint(100.0f), makePoint(500.0f) };
    TEST_ASSERT_TRUE(SolarMath::hasValidSolarData(pts, 2));
}

void test_has_valid_data_too_few_points(void) {
    WeatherHourlyForecast pts[] = { makePoint(500.0f) };
    TEST_ASSERT_FALSE(SolarMath::hasValidSolarData(pts, 1));
}

void test_has_valid_data_all_unavailable(void) {
    WeatherHourlyForecast pts[] = { makePoint(-1.0f), makePoint(-1.0f) };
    TEST_ASSERT_FALSE(SolarMath::hasValidSolarData(pts, 2));
}

void test_has_valid_data_all_night_zero_is_false(void) {
    // All-zero (night) has valid points but zero peak -> cannot draw a meaningful
    // curve, so hasValidSolarData is false (caller falls back).
    WeatherHourlyForecast pts[] = { makePoint(0.0f), makePoint(0.0f) };
    TEST_ASSERT_FALSE(SolarMath::hasValidSolarData(pts, 2));
}

void test_has_valid_data_single_valid_among_unavailable(void) {
    // Only one valid point -> not enough to draw a line.
    WeatherHourlyForecast pts[] = { makePoint(-1.0f), makePoint(500.0f), makePoint(-1.0f) };
    TEST_ASSERT_FALSE(SolarMath::hasValidSolarData(pts, 3));
}

// ---------------------------------------------------------------------------
// normalizeToPercent — mapping radiation to 0..100% of axis
// ---------------------------------------------------------------------------
void test_normalize_basic(void) {
    // 300 of a 600 axis = 50%.
    TEST_ASSERT_EQUAL_FLOAT(50.0f, SolarMath::normalizeToPercent(300.0f, 600.0f));
    TEST_ASSERT_EQUAL_FLOAT(100.0f, SolarMath::normalizeToPercent(600.0f, 600.0f));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, SolarMath::normalizeToPercent(0.0f, 600.0f));
}

void test_normalize_clamps_over_100(void) {
    // A value above the axis max (rare cloud-enhancement) clamps to 100%.
    TEST_ASSERT_EQUAL_FLOAT(100.0f, SolarMath::normalizeToPercent(700.0f, 600.0f));
}

void test_normalize_unavailable_is_zero(void) {
    TEST_ASSERT_EQUAL_FLOAT(0.0f, SolarMath::normalizeToPercent(-1.0f, 600.0f));
}

void test_normalize_guards_bad_axis(void) {
    // axisMax <= 0 must not divide by zero.
    TEST_ASSERT_EQUAL_FLOAT(0.0f, SolarMath::normalizeToPercent(300.0f, 0.0f));
}

int main(int, char**) {
    UNITY_BEGIN();

    RUN_TEST(test_axis_max_rounds_up_to_next_100);
    RUN_TEST(test_axis_max_exact_multiple_stays);
    RUN_TEST(test_axis_max_floor_for_near_dark);
    RUN_TEST(test_axis_max_high_summer);

    RUN_TEST(test_find_peak_basic);
    RUN_TEST(test_find_peak_skips_unavailable);
    RUN_TEST(test_find_peak_all_unavailable);
    RUN_TEST(test_find_peak_all_zero_night);

    RUN_TEST(test_has_valid_data_true);
    RUN_TEST(test_has_valid_data_too_few_points);
    RUN_TEST(test_has_valid_data_all_unavailable);
    RUN_TEST(test_has_valid_data_all_night_zero_is_false);
    RUN_TEST(test_has_valid_data_single_valid_among_unavailable);

    RUN_TEST(test_normalize_basic);
    RUN_TEST(test_normalize_clamps_over_100);
    RUN_TEST(test_normalize_unavailable_is_zero);
    RUN_TEST(test_normalize_guards_bad_axis);

    return UNITY_END();
}
