#pragma once
#include <Arduino.h>

// Configuration phase tracking
enum ConfigPhase {
    PHASE_WIFI_SETUP = 1, // WiFi credentials needed
    PHASE_APP_SETUP = 2, // WiFi OK, app settings needed
    PHASE_COMPLETE = 3 // All configured
};

