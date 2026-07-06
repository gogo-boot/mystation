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

Each device adds a **deterministic, device-unique offset** to its sleep duration on the
**first wake cycle after boot**. This one-time jitter spreads devices apart, and since each
device then follows its own interval from that shifted starting point, they remain spread
across subsequent cycles without further jitter.

### Key Properties

| Property | Value |
|----------|-------|
| Jitter source | MAC address (unique per device) |
| Jitter range | 0 to `MAX_JITTER_SECONDS - 1` (default: 0–59 seconds) |
| Deterministic | Yes — same device always gets the same offset |
| Applied | Only once per boot (when both `lastWeatherUpdate` and `lastTransportUpdate` are 0) |
| Coordination needed | None — devices operate independently |
| Battery impact | None — only adds sleep on first cycle |

### Why Only on First Wake?

If jitter were applied every cycle, it would accumulate and drift the update interval:

```
❌ Jitter every cycle (interval=300s, jitter=59s):
  Cycle 1: sleeps 359s, wakes 59s late
  Cycle 2: sleeps 359s, wakes 118s late
  Cycle 3: sleeps 359s, wakes 177s late
  ...after 5 cycles: 5 minutes behind schedule!

✅ Jitter only on first wake:
  Cycle 1: sleeps 359s (one-time offset)
  Cycle 2: sleeps 300s (exact interval)
  Cycle 3: sleeps 300s (exact interval)
  ...permanently offset by 59s, interval stays exact
```

The first-wake-only approach is correct because:
- After power loss, all devices boot simultaneously → thundering herd → jitter needed
- After the first cycle, `lastUpdate` is set → each device's schedule is already shifted
- Subsequent cycles use exact intervals, no drift

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

The jitter is added to the final sleep duration **only on the first wake after boot**:

```cpp
// At the end of getNextSleepDurationSeconds():
if (getLastWeatherUpdate() == 0 && getLastTransportUpdate() == 0) {
    uint32_t jitter = getDeviceJitterSeed() % MAX_JITTER_SECONDS;
    sleepSeconds += jitter;
}
```

The condition `lastWeatherUpdate == 0 && lastTransportUpdate == 0` is true only when:
- Device has just cold-booted (power loss, reset, OTA reboot)
- RTC memory was cleared (these are `RTC_DATA_ATTR` variables)

After the first API fetch, at least one `lastUpdate` is set → jitter is never applied again
until the next cold boot.

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
- The first wake after boot is delayed by up to 60 seconds
- All subsequent updates follow the configured interval exactly
- No drift or accumulation over time

## Testing

In native tests, `getDeviceJitterSeed()` returns `0xDEADBEEF`, producing a deterministic
jitter of `0xDEADBEEF % 60 = 59` seconds.

Four dedicated jitter tests verify:
1. Jitter applies on first wake (both `lastUpdate == 0`)
2. Jitter does NOT apply when weather has been updated
3. Jitter does NOT apply when transport has been updated
4. Jitter seed is deterministic

Run tests with:

```bash
pio test -e native -v
```

## Related Files

| File | Description |
|------|-------------|
| `include/util/timing_manager.h` | `MAX_JITTER_SECONDS` constant, `getDeviceJitterSeed()` declaration |
| `src/util/timing_manager.cpp` | Jitter implementation in `getNextSleepDurationSeconds()` |
| `test/test_timing_manager/test_sleep_duration.cpp` | Tests verifying jitter behavior |
| `docs/developer-guide/boot-process.md` | Sleep duration calculation overview |
