#pragma once
#include "config/config_struct.h"

// Forward declarations for data structures
struct WeatherInfo;
struct DepartureData;

/**
 * DeviceModeManager - Orchestrates boot phases and operational modes
 *
 * Responsibilities:
 * - Determine current configuration phase (WiFi/App/Complete)
 * - Execute phase-specific logic (WiFi AP, config portal, operational mode)
 * - Fetch data and render display in operational mode
 */
class DeviceModeManager {
public:
    // Phase handlers (called by ActivityManager)
    static void handlePhaseWifiSetup();
    static void handlePhaseAppSetup();
    static void handlePhaseComplete();

    // Configuration phase detection
    static ConfigPhase getCurrentPhase();

    // Network & time setup
    static bool setupConnectivityAndTime();

private:
    // Phase 1 helpers
    static void showPhaseInstructions(ConfigPhase phase);
    static void logWifiError();

    // Phase 2 helpers
    static void runConfigurationMode();
    static void startWebServer();

    // Phase 3 (operational) helpers
    static void runOperationalMode(uint8_t displayMode);
    static void showWeatherDeparture();
    static void updateWeatherFull();
    static void updateDepartureFull();
    static void showApplicationInfo();
    static bool fetchTransportData(DepartureData& depart);
};
