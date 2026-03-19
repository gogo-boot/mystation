#pragma once
#include <Arduino.h>

// =============================================================================
// ButtonMonitor - Unified long-press detection for all buttons
// =============================================================================
// Returns a bitmask of which buttons were held for the full duration.
// Use the BUTTON_* constants below to check individual bits.
// =============================================================================

// Bitmask constants
constexpr uint8_t BUTTON_NONE = 0b00000000; ///< No button held
constexpr uint8_t BUTTON_1_HELD = 0b00000001; ///< Button 1 held (GPIO_BUTTON_1)
constexpr uint8_t BUTTON_2_HELD = 0b00000010; ///< Button 2 held (GPIO_BUTTON_2)
constexpr uint8_t BUTTON_3_HELD = 0b00000100; ///< Button 3 held (GPIO_BUTTON_3)
constexpr uint8_t BUTTON_RESERVED = 0b00001000; ///< Reserved for future use

// Combination helpers
constexpr uint8_t BUTTON_1_AND_2 = BUTTON_1_HELD | BUTTON_2_HELD; ///< Both button 1 & 2 held

class ButtonMonitor {
public:
    /**
     * Poll all buttons simultaneously and return a bitmask of which buttons
     * were held continuously for the full @p durationMs milliseconds.
     *
     * @param durationMs  How long (ms) a button must be held to be counted.
     * @return            Bitmask of BUTTON_*_HELD flags, or BUTTON_NONE.
     */
    static uint8_t detectLongPress(unsigned long durationMs);
};

