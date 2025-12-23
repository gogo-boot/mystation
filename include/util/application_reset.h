#pragma once
#include <Arduino.h>

// Hold duration in milliseconds to trigger factory reset
const unsigned long APPLICATION_RESET_HOLD_DURATION_MS = 3000;


class AppicationReset {
public:
    static boolean checkResetButton();

    static void performReset();
};
