# Jittered Sleep Duration (Thundering Herd Prevention)

## Problem

When thousands of MyStation devices are deployed, they tend to wake up at similar intervals
and call the weather API (Open-Meteo/DWD) and transport API (RMV) simultaneously. This creates
request spikes that can overwhelm API servers or trigger rate limiting.

```
Without jitter (1000 devices, 5-min interval):
  Requests │████████████████████████████  ← 1000 requests in <2s
           └──────────────────────────── time

With 60s jitter:
  Requests │█ █ ██ █ █ ██ █ █ ██ █ █ █  ← ~17 requests/second
           └──────────────────────────── time (spread over 60s)
```

## Solution

Each device adds a **deterministic, device-unique offset** to its sleep duration. This spreads
wake-up times evenly across a configurable jitter window without requiring any coordination
between devices.

### Key Properties

| Property | Value |
|----------|-------|
| Jitter source | MAC address (unique per device) |
| Jitter range | 0 to `MAX_JITTER_SECONDS - 1` (default: 0–59 seconds) |
| Deterministic | Yes — same device always gets the same offset |
| Coordination needed | None — devices operate independently |
| Battery impact | None — jitter extends sleep (less wake time) |

## Implementation

### Jitter Seed (`getDeviceJitterSeed()`)

The jitter seed is derived from the device's factory-programmed MAC address:

```cpp
uint32_t TimingManager::getDeviceJitterSeed() {
#ifdef NATIVE_TEST
    return 0xDEADBEEF; // Fixed seed for reproducible tests
#else
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    return (uint32_t)(mac[2] << 24) | (mac[3] << 16) | (mac[4] << 8) | mac[5];
#endif
}
```

**Why MAC address?**
- Burned into eFuse at factory — guaranteed unique per device
- Available without WiFi connection or NVS access
- Zero runtime cost (no random number generation)
- Deterministic: user sees consistent wake times

### Jitter Application

The jitter is added to the final sleep duration after the rule-based candidate selection:

```cpp
// At the end of getNextSleepDurationSeconds():
uint32_t jitter = getDeviceJitterSeed() % MAX_JITTER_SECONDS;
sleepSeconds += jitter;
```

The jitter is **not** applied to:
- Temporary mode early returns (button press → 2-minute cycle)
- Overdue candidate returns (immediate wake = minimum 30s)

### Configuration

The maximum jitter window is defined in `include/util/timing_manager.h`:

```cpp
static constexpr uint32_t MAX_JITTER_SECONDS = 60;
```

## Scaling Recommendations

| Device Count | Recommended `MAX_JITTER_SECONDS` | Avg Requests/Second |
|---|---|---|
| 100 | 30 | ~3.3 |
| 1,000 | 60 | ~17 |
| 10,000 | 120–180 | ~56–83 |
| 50,000+ | Consider API proxy/cache | — |

For deployments above 10,000 devices, consider adding a caching proxy (e.g., CloudFront + Lambda)
between devices and upstream APIs to:
- Absorb request bursts
- Reduce dependency on external API availability
- Cache identical responses (weather data is the same for nearby devices)

## Effect on Display Update Timing

From the user's perspective, the jitter is invisible:
- A device configured for 5-minute transport updates will actually update every 5 minutes + jitter
- With the default 60s jitter, updates arrive between 5:00 and 5:59 minutes — imperceptible

The jitter does **not** accumulate across cycles. Each sleep duration is independently calculated
as `base_duration + fixed_jitter`, so the offset remains constant.

## Testing

In native tests, `getDeviceJitterSeed()` returns `0xDEADBEEF`, producing a deterministic
jitter of `0xDEADBEEF % 60 = 59` seconds. Test assertions account for this:

```cpp
// In test_sleep_duration.cpp:
static const uint32_t EXPECTED_JITTER = TimingManager::getDeviceJitterSeed() % MAX_JITTER_SECONDS;

// Assertions include jitter:
TEST_ASSERT_EQUAL(3600 + EXPECTED_JITTER, sleepDuration); // 1 hour + jitter
```

Run tests with:

```bash
pio test -e native -v
```

## Related Files

| File | Description |
|------|-------------|
| `include/util/timing_manager.h` | `MAX_JITTER_SECONDS` constant, `getDeviceJitterSeed()` declaration |
| `src/util/timing_manager.cpp` | Jitter implementation in `getNextSleepDurationSeconds()` |
| `test/test_timing_manager/test_sleep_duration.cpp` | Tests with `EXPECTED_JITTER` constant |
| `docs/developer-guide/boot-process.md` | Sleep duration calculation overview |
