/**
 * test_weather_browse.cpp
 *
 * Unit tests for the weather-browse state machine
 * (ButtonManager::applyWeatherBrowsePress + stepBrowseDay).
 *
 * The real function is a private static method with hardware dependencies
 * (Preferences/NVS, esp_sleep, TimingManager::getEffectiveDisplayMode which reads
 * the clock). Following the same pattern as test_button_wakeup_mode, this suite
 * reimplements the pure decision logic in a self-contained way and verifies it,
 * so the enter/step/wrap/exit behavior is locked against regressions.
 *
 * The logic mirrored here must stay in sync with:
 *   src/util/button_manager.cpp :: stepBrowseDay() and applyWeatherBrowsePress()
 */

#include <unity.h>
#include <cstdint>

// ── Mirror of the production enums / fields the logic touches ──────────────
enum { DISPLAY_MODE_HALF_AND_HALF = 0, DISPLAY_MODE_WEATHER_ONLY = 1,
       DISPLAY_MODE_TRANSPORT_ONLY = 2, DISPLAY_MODE_APPLICATION_INFO = 3 };
enum { BROWSE_WEATHER = 0, BROWSE_SOLAR = 1 };
enum BrowseResult { BROWSE_NONE = 0, BROWSE_ENTER_OR_STEP, BROWSE_EXIT };

// Minimal stand-in for the browse-relevant parts of RTCConfigData.
struct Cfg {
    uint8_t displayMode;
    int8_t  selectedForecastDay;
    int8_t  availableForecastDays;
    uint8_t browseContext;
    bool    inWeatherBrowse;
    // effective display mode is injected by the test (stands in for
    // TimingManager::getEffectiveDisplayMode()).
    uint8_t effectiveMode;
};

// ── Mirror of stepBrowseDay() (button_manager.cpp) ─────────────────────────
static int8_t stepBrowseDay(const Cfg& c, int delta) {
    int8_t maxDays = c.availableForecastDays > 1 ? c.availableForecastDays : 7;
    int8_t lower = (c.browseContext == BROWSE_SOLAR) ? 0 : 1;
    int8_t upper = maxDays - 1;
    if (upper < lower) upper = lower;
    int span = upper - lower + 1;
    int8_t cur = c.selectedForecastDay;
    if (cur < lower) cur = lower;
    if (cur > upper) cur = upper;
    int idx = (cur - lower + delta) % span;
    if (idx < 0) idx += span;
    return (int8_t)(lower + idx);
}

// ── Mirror of applyWeatherBrowsePress() (button_manager.cpp) ───────────────
static BrowseResult applyWeatherBrowsePress(Cfg& c, int8_t buttonMode) {
    if (c.effectiveMode != DISPLAY_MODE_WEATHER_ONLY
        || buttonMode == DISPLAY_MODE_APPLICATION_INFO) {
        return BROWSE_NONE;
    }
    bool configuredWeatherOnly = (c.displayMode == DISPLAY_MODE_WEATHER_ONLY);
    if (!c.inWeatherBrowse) {
        if (buttonMode == DISPLAY_MODE_WEATHER_ONLY) {
            c.inWeatherBrowse = true;
            c.browseContext = BROWSE_WEATHER;
            c.selectedForecastDay = 1;
            return BROWSE_ENTER_OR_STEP;
        }
        if (configuredWeatherOnly) {
            if (buttonMode == DISPLAY_MODE_TRANSPORT_ONLY) {
                c.inWeatherBrowse = true;
                c.browseContext = BROWSE_SOLAR;
                c.selectedForecastDay = 0;
                return BROWSE_ENTER_OR_STEP;
            }
            // B1 -> stay weather-today
            c.inWeatherBrowse = false;
            c.selectedForecastDay = 0;
            c.browseContext = BROWSE_WEATHER;
            return BROWSE_EXIT;
        }
        return BROWSE_NONE; // Half & Half: B1/B3 normal mode switch
    }
    if (buttonMode == DISPLAY_MODE_WEATHER_ONLY) {
        c.selectedForecastDay = stepBrowseDay(c, +1);
        return BROWSE_ENTER_OR_STEP;
    } else if (buttonMode == DISPLAY_MODE_TRANSPORT_ONLY) {
        c.selectedForecastDay = stepBrowseDay(c, -1);
        return BROWSE_ENTER_OR_STEP;
    } else { // B1
        c.inWeatherBrowse = false;
        c.selectedForecastDay = 0;
        c.browseContext = BROWSE_WEATHER;
        return BROWSE_EXIT;
    }
}

static Cfg makeCfg(uint8_t configured, uint8_t effective) {
    Cfg c;
    c.displayMode = configured;
    c.selectedForecastDay = 0;
    c.availableForecastDays = 7;
    c.browseContext = BROWSE_WEATHER;
    c.inWeatherBrowse = false;
    c.effectiveMode = effective;
    return c;
}

// ── Guard: no action unless Weather-Full is effectively showing ────────────
void test_no_browse_when_effective_half_and_half() {
    // Configured Half&Half, transport hours -> effective Half&Half. B2 must NOT browse.
    Cfg c = makeCfg(DISPLAY_MODE_HALF_AND_HALF, DISPLAY_MODE_HALF_AND_HALF);
    TEST_ASSERT_EQUAL(BROWSE_NONE, applyWeatherBrowsePress(c, DISPLAY_MODE_WEATHER_ONLY));
    TEST_ASSERT_FALSE(c.inWeatherBrowse);
}

void test_app_info_never_browses() {
    Cfg c = makeCfg(DISPLAY_MODE_WEATHER_ONLY, DISPLAY_MODE_WEATHER_ONLY);
    TEST_ASSERT_EQUAL(BROWSE_NONE, applyWeatherBrowsePress(c, DISPLAY_MODE_APPLICATION_INFO));
    TEST_ASSERT_FALSE(c.inWeatherBrowse);
}

// ── Enter ──────────────────────────────────────────────────────────────────
void test_b2_enters_browse_at_day1() {
    Cfg c = makeCfg(DISPLAY_MODE_HALF_AND_HALF, DISPLAY_MODE_WEATHER_ONLY);
    BrowseResult r = applyWeatherBrowsePress(c, DISPLAY_MODE_WEATHER_ONLY);
    TEST_ASSERT_EQUAL(BROWSE_ENTER_OR_STEP, r);
    TEST_ASSERT_TRUE(c.inWeatherBrowse);
    TEST_ASSERT_EQUAL(BROWSE_WEATHER, c.browseContext);
    TEST_ASSERT_EQUAL(1, c.selectedForecastDay);
}

void test_b1_b3_do_not_enter_browse() {
    // Not browsing, weather-full showing: B1 and B3 are NOT browse actions.
    Cfg c = makeCfg(DISPLAY_MODE_HALF_AND_HALF, DISPLAY_MODE_WEATHER_ONLY);
    TEST_ASSERT_EQUAL(BROWSE_NONE, applyWeatherBrowsePress(c, DISPLAY_MODE_HALF_AND_HALF));
    TEST_ASSERT_FALSE(c.inWeatherBrowse);
    TEST_ASSERT_EQUAL(BROWSE_NONE, applyWeatherBrowsePress(c, DISPLAY_MODE_TRANSPORT_ONLY));
    TEST_ASSERT_FALSE(c.inWeatherBrowse);
}

// ── Step forward / backward ────────────────────────────────────────────────
void test_b2_steps_forward() {
    Cfg c = makeCfg(DISPLAY_MODE_HALF_AND_HALF, DISPLAY_MODE_WEATHER_ONLY);
    applyWeatherBrowsePress(c, DISPLAY_MODE_WEATHER_ONLY); // enter -> day 1
    applyWeatherBrowsePress(c, DISPLAY_MODE_WEATHER_ONLY); // +1 -> day 2
    TEST_ASSERT_EQUAL(2, c.selectedForecastDay);
    applyWeatherBrowsePress(c, DISPLAY_MODE_WEATHER_ONLY); // +1 -> day 3
    TEST_ASSERT_EQUAL(3, c.selectedForecastDay);
}

void test_b3_steps_backward() {
    Cfg c = makeCfg(DISPLAY_MODE_WEATHER_ONLY, DISPLAY_MODE_WEATHER_ONLY);
    applyWeatherBrowsePress(c, DISPLAY_MODE_WEATHER_ONLY); // enter -> day 1
    // B3 from day 1 wraps backward to the last day (6 for 7-day model)
    applyWeatherBrowsePress(c, DISPLAY_MODE_TRANSPORT_ONLY);
    TEST_ASSERT_EQUAL(6, c.selectedForecastDay);
}

// ── Circular wrap (weather browse skips day 0: range 1..max-1) ─────────────
void test_forward_wraps_to_1() {
    Cfg c = makeCfg(DISPLAY_MODE_WEATHER_ONLY, DISPLAY_MODE_WEATHER_ONLY);
    c.inWeatherBrowse = true;
    c.selectedForecastDay = 6; // last day (7-day model)
    applyWeatherBrowsePress(c, DISPLAY_MODE_WEATHER_ONLY); // +1 -> wraps to 1
    TEST_ASSERT_EQUAL(1, c.selectedForecastDay);
}

void test_backward_wraps_to_max() {
    Cfg c = makeCfg(DISPLAY_MODE_WEATHER_ONLY, DISPLAY_MODE_WEATHER_ONLY);
    c.inWeatherBrowse = true;
    c.selectedForecastDay = 1;
    applyWeatherBrowsePress(c, DISPLAY_MODE_TRANSPORT_ONLY); // -1 -> wraps to 6
    TEST_ASSERT_EQUAL(6, c.selectedForecastDay);
}

void test_wrap_respects_limited_day_model() {
    // ItaliaMeteo-style 3-day model: browsable range is day 1..2.
    Cfg c = makeCfg(DISPLAY_MODE_WEATHER_ONLY, DISPLAY_MODE_WEATHER_ONLY);
    c.availableForecastDays = 3; // days 0..2 -> browse 1..2
    c.inWeatherBrowse = true;
    c.selectedForecastDay = 2; // last browsable
    applyWeatherBrowsePress(c, DISPLAY_MODE_WEATHER_ONLY); // +1 -> wraps to 1
    TEST_ASSERT_EQUAL(1, c.selectedForecastDay);
    applyWeatherBrowsePress(c, DISPLAY_MODE_TRANSPORT_ONLY); // -1 -> wraps to 2
    TEST_ASSERT_EQUAL(2, c.selectedForecastDay);
}

// ── Exit ────────────────────────────────────────────────────────────────────
void test_b1_exits_browse() {
    Cfg c = makeCfg(DISPLAY_MODE_HALF_AND_HALF, DISPLAY_MODE_WEATHER_ONLY);
    applyWeatherBrowsePress(c, DISPLAY_MODE_WEATHER_ONLY); // enter -> day 1
    applyWeatherBrowsePress(c, DISPLAY_MODE_WEATHER_ONLY); // -> day 2
    BrowseResult r = applyWeatherBrowsePress(c, DISPLAY_MODE_HALF_AND_HALF); // B1 exit
    TEST_ASSERT_EQUAL(BROWSE_EXIT, r);
    TEST_ASSERT_FALSE(c.inWeatherBrowse);
    TEST_ASSERT_EQUAL(0, c.selectedForecastDay);
    TEST_ASSERT_EQUAL(BROWSE_WEATHER, c.browseContext);
}

// ── Two-step Half&Half flow (transport hours) ──────────────────────────────
void test_half_and_half_two_step_enter() {
    // Transport hours: configured Half&Half, effective Half&Half.
    Cfg c = makeCfg(DISPLAY_MODE_HALF_AND_HALF, DISPLAY_MODE_HALF_AND_HALF);
    // First B2 -> NONE (caller switches to temp Weather-Full).
    TEST_ASSERT_EQUAL(BROWSE_NONE, applyWeatherBrowsePress(c, DISPLAY_MODE_WEATHER_ONLY));
    TEST_ASSERT_FALSE(c.inWeatherBrowse);
    // Now weather-full is showing -> effective becomes Weather-Only.
    c.effectiveMode = DISPLAY_MODE_WEATHER_ONLY;
    // Second B2 -> enters browse.
    TEST_ASSERT_EQUAL(BROWSE_ENTER_OR_STEP, applyWeatherBrowsePress(c, DISPLAY_MODE_WEATHER_ONLY));
    TEST_ASSERT_TRUE(c.inWeatherBrowse);
    TEST_ASSERT_EQUAL(1, c.selectedForecastDay);
}

// ── Configured Weather-Only: B3 = solar, B1 = stay-today, never a transport view ──
void test_weatheronly_b3_enters_solar() {
    Cfg c = makeCfg(DISPLAY_MODE_WEATHER_ONLY, DISPLAY_MODE_WEATHER_ONLY);
    BrowseResult r = applyWeatherBrowsePress(c, DISPLAY_MODE_TRANSPORT_ONLY);
    TEST_ASSERT_EQUAL(BROWSE_ENTER_OR_STEP, r);     // consumed, NOT a transport switch
    TEST_ASSERT_TRUE(c.inWeatherBrowse);
    TEST_ASSERT_EQUAL(BROWSE_SOLAR, c.browseContext);
    TEST_ASSERT_EQUAL(0, c.selectedForecastDay);    // solar includes today
}

void test_weatheronly_b1_stays_today() {
    Cfg c = makeCfg(DISPLAY_MODE_WEATHER_ONLY, DISPLAY_MODE_WEATHER_ONLY);
    BrowseResult r = applyWeatherBrowsePress(c, DISPLAY_MODE_HALF_AND_HALF);
    TEST_ASSERT_EQUAL(BROWSE_EXIT, r);              // consumed as "stay today", NOT a split switch
    TEST_ASSERT_FALSE(c.inWeatherBrowse);
    TEST_ASSERT_EQUAL(0, c.selectedForecastDay);
}

void test_weatheronly_never_returns_none() {
    // The bug: in configured Weather-Only, B1/B3 fell through to transport views.
    // No button press may return BROWSE_NONE (which triggers a mode switch).
    for (int8_t b = 0; b <= 2; b++) {
        Cfg c = makeCfg(DISPLAY_MODE_WEATHER_ONLY, DISPLAY_MODE_WEATHER_ONLY);
        TEST_ASSERT_NOT_EQUAL(BROWSE_NONE, applyWeatherBrowsePress(c, b));
    }
}

void test_weatheronly_solar_step_includes_day0() {
    // In solar browse, backward from day 0 wraps to the last day; forward returns to 0.
    Cfg c = makeCfg(DISPLAY_MODE_WEATHER_ONLY, DISPLAY_MODE_WEATHER_ONLY);
    applyWeatherBrowsePress(c, DISPLAY_MODE_TRANSPORT_ONLY); // enter solar, day 0
    applyWeatherBrowsePress(c, DISPLAY_MODE_WEATHER_ONLY);   // +1 -> day 1
    TEST_ASSERT_EQUAL(1, c.selectedForecastDay);
    c.selectedForecastDay = 0;
    applyWeatherBrowsePress(c, DISPLAY_MODE_TRANSPORT_ONLY); // -1 from 0 -> wraps to 6
    TEST_ASSERT_EQUAL(6, c.selectedForecastDay);
}

void test_halfhalf_b1_b3_still_normal_switch() {
    // In configured Half & Half (weather-full showing, not browsing), B1/B3 remain
    // normal mode switches (BROWSE_NONE) — unchanged by the Weather-Only fix.
    Cfg c = makeCfg(DISPLAY_MODE_HALF_AND_HALF, DISPLAY_MODE_WEATHER_ONLY);
    TEST_ASSERT_EQUAL(BROWSE_NONE, applyWeatherBrowsePress(c, DISPLAY_MODE_HALF_AND_HALF));
    Cfg c2 = makeCfg(DISPLAY_MODE_HALF_AND_HALF, DISPLAY_MODE_WEATHER_ONLY);
    TEST_ASSERT_EQUAL(BROWSE_NONE, applyWeatherBrowsePress(c2, DISPLAY_MODE_TRANSPORT_ONLY));
}

void setUp() {}
void tearDown() {}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_no_browse_when_effective_half_and_half);
    RUN_TEST(test_app_info_never_browses);
    RUN_TEST(test_b2_enters_browse_at_day1);
    RUN_TEST(test_b1_b3_do_not_enter_browse);
    RUN_TEST(test_b2_steps_forward);
    RUN_TEST(test_b3_steps_backward);
    RUN_TEST(test_forward_wraps_to_1);
    RUN_TEST(test_backward_wraps_to_max);
    RUN_TEST(test_wrap_respects_limited_day_model);
    RUN_TEST(test_b1_exits_browse);
    RUN_TEST(test_half_and_half_two_step_enter);
    RUN_TEST(test_weatheronly_b3_enters_solar);
    RUN_TEST(test_weatheronly_b1_stays_today);
    RUN_TEST(test_weatheronly_never_returns_none);
    RUN_TEST(test_weatheronly_solar_step_includes_day0);
    RUN_TEST(test_halfhalf_b1_b3_still_normal_switch);
    return UNITY_END();
}
