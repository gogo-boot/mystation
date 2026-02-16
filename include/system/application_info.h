#pragma once
#include <Arduino.h>

// Hold duration in milliseconds to trigger factory reset
const unsigned long APPLICATION_INFO_HOLD_DURATION_MS = 3000;

class AppicationInfo {
public:
    static boolean checkButtonHold(int buttonPin);

    static void showApplicationInfo();
};
