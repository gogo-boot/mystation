#pragma once

// =============================================================================
// GPIO Pin Definitions - Dual Layer Naming Strategy
// =============================================================================
// Layer 1: Physical GPIO assignments (GPIO_* prefix)
// Layer 2: Functional aliases for application code (EPD_*, BUTTON_*, etc.)
//
// Benefits:
// - Physical layer: Clear hardware GPIO numbers for debugging
// - Functional layer: Readable, self-documenting code
// - Easy board portability and future hardware changes
//
// Board Identification Convention: BOARD_{chip}_{product}
// -------------------------------------------------------
// Define              | Chip     | Product               | Set in platformio.ini
// --------------------|----------|-----------------------|----------------------
// BOARD_C3_SUPERMINI  | ESP32-C3 | ESP32-C3 Super Mini   | esp32-c3-* envs
// BOARD_C5_XIAO      | ESP32-C5 | Seeed XIAO ESP32-C5   | (future)
// BOARD_S3_E1001     | ESP32-S3 | TRMNL E1001 PCB       | esp32-s3-e1001-* envs
// BOARD_S3_EE04      | ESP32-S3 | TRMNL EE04 PCB        | esp32-s3-ee04-* envs
// =============================================================================

namespace Pins {
    // =========================================================================
    // LAYER 1: Physical GPIO Pin Assignments
    // =========================================================================

#ifdef BOARD_C3_SUPERMINI
    // --- E-Paper Display Hardware Pins ---
    constexpr gpio_num_t GPIO_EPD_BUSY = GPIO_NUM_2;
    constexpr gpio_num_t GPIO_EPD_CS = GPIO_NUM_3;
    constexpr gpio_num_t GPIO_EPD_SCK = GPIO_NUM_4;
    constexpr gpio_num_t GPIO_EPD_SDI = GPIO_NUM_6;
    constexpr gpio_num_t GPIO_EPD_RES = GPIO_NUM_8;
    constexpr gpio_num_t GPIO_EPD_DC = GPIO_NUM_9;

    // --- Button Pins (Available for future use) ---
    constexpr gpio_num_t GPIO_BUTTON_1 = GPIO_NUM_0; // Available for button 1
    constexpr gpio_num_t GPIO_BUTTON_2 = GPIO_NUM_1; // Available for button 2
    constexpr gpio_num_t GPIO_BUTTON_3 = GPIO_NUM_5; // Available for button 3

#elif defined(BOARD_S3_E1001)
    // --- E-Paper Display Hardware Pins ---
    constexpr gpio_num_t GPIO_EPD_BUSY = GPIO_NUM_13;
    constexpr gpio_num_t GPIO_EPD_CS = GPIO_NUM_10;
    constexpr gpio_num_t GPIO_EPD_SCK = GPIO_NUM_7;
    constexpr gpio_num_t GPIO_EPD_SDI = GPIO_NUM_9;
    constexpr gpio_num_t GPIO_EPD_RES = GPIO_NUM_12;
    constexpr gpio_num_t GPIO_EPD_DC = GPIO_NUM_11;

    // --- Battery Management Hardware Pins ---
    // Reference: https://wiki.seeedstudio.com/ogdiy_kit_works_with_arduino/
    constexpr gpio_num_t GPIO_BATTERY_ADC = GPIO_NUM_1; // Voltage sense (2:1 divider)
    constexpr gpio_num_t GPIO_BATTERY_ADC_EN = GPIO_NUM_21; // ADC power control

    // --- I2C Pins (SHT4x Indoor Sensor) ---
    constexpr gpio_num_t GPIO_I2C_SDA = GPIO_NUM_19;
    constexpr gpio_num_t GPIO_I2C_SCL = GPIO_NUM_20;

    // --- Button Hardware Pins ---
    constexpr gpio_num_t GPIO_BUTTON_1 = GPIO_NUM_3; // Multi-function button
    constexpr gpio_num_t GPIO_BUTTON_2 = GPIO_NUM_4;
    constexpr gpio_num_t GPIO_BUTTON_3 = GPIO_NUM_5;

#elif defined(BOARD_S3_EE04)
    // --- E-Paper Display Hardware Pins ---
    constexpr gpio_num_t GPIO_EPD_BUSY = GPIO_NUM_4;
    constexpr gpio_num_t GPIO_EPD_CS = GPIO_NUM_44;
    constexpr gpio_num_t GPIO_EPD_SCK = GPIO_NUM_7;
    constexpr gpio_num_t GPIO_EPD_SDI = GPIO_NUM_9;
    constexpr gpio_num_t GPIO_EPD_RES = GPIO_NUM_38;
    constexpr gpio_num_t GPIO_EPD_DC = GPIO_NUM_10;

    // --- Battery Management Hardware Pins ---
    // Reference: https://wiki.seeedstudio.com/ogdiy_kit_works_with_arduino/
    constexpr gpio_num_t GPIO_BATTERY_ADC = GPIO_NUM_1; // Voltage sense (2:1 divider)
    constexpr gpio_num_t GPIO_BATTERY_ADC_EN = GPIO_NUM_6; // ADC power control

    // --- Button Hardware Pins ---
    constexpr gpio_num_t GPIO_BUTTON_1 = GPIO_NUM_2; // Multi-function button
    constexpr gpio_num_t GPIO_BUTTON_2 = GPIO_NUM_3;
    constexpr gpio_num_t GPIO_BUTTON_3 = GPIO_NUM_5;

#elif defined(BOARD_C5_XIAO)
    // --- E-Paper Display Hardware Pins ---
    constexpr gpio_num_t GPIO_EPD_BUSY = GPIO_NUM_2;
    constexpr gpio_num_t GPIO_EPD_CS = GPIO_NUM_7;
    constexpr gpio_num_t GPIO_EPD_SCK = GPIO_NUM_8;
    constexpr gpio_num_t GPIO_EPD_SDI = GPIO_NUM_10;
    constexpr gpio_num_t GPIO_EPD_RES = GPIO_NUM_0;
    constexpr gpio_num_t GPIO_EPD_DC = GPIO_NUM_1;

    // --- Battery Management Hardware Pins ---
    // Reference: https://wiki.seeedstudio.com/ogdiy_kit_works_with_arduino/
    constexpr gpio_num_t GPIO_BATTERY_ADC = GPIO_NUM_1; // Voltage sense (2:1 divider)
    constexpr gpio_num_t GPIO_BATTERY_ADC_EN = GPIO_NUM_6; // ADC power control

    // --- Button Hardware Pins ---
    constexpr gpio_num_t GPIO_BUTTON_1 = GPIO_NUM_2; // Multi-function button
    constexpr gpio_num_t GPIO_BUTTON_2 = GPIO_NUM_3;
    constexpr gpio_num_t GPIO_BUTTON_3 = GPIO_NUM_5;

#else
#error "Board not defined! Please specify board type in platformio.ini"
#endif

    // =========================================================================
    // LAYER 2: Functional Pin Aliases (Use these in application code)
    // =========================================================================
    // --- E-Paper Display Functional Names ---
    constexpr int EPD_BUSY = GPIO_EPD_BUSY;
    constexpr int EPD_CS = GPIO_EPD_CS;
    constexpr int EPD_SCK = GPIO_EPD_SCK;
    constexpr int EPD_SDI = GPIO_EPD_SDI;
    constexpr int EPD_RES = GPIO_EPD_RES;
    constexpr int EPD_DC = GPIO_EPD_DC;

#if defined(BOARD_S3_E1001) || defined(BOARD_S3_EE04)
    // --- Battery Management Functional Names ---
    constexpr int BATTERY_ADC = GPIO_BATTERY_ADC;
    constexpr int ADC_EN = GPIO_BATTERY_ADC_EN;

    // --- Button Functional Names ---
    constexpr int BUTTON_HALF_AND_HALF = GPIO_BUTTON_1; // Weather + Departures mode
    constexpr int BUTTON_WEATHER_ONLY = GPIO_BUTTON_2; // Weather only mode
    constexpr int BUTTON_DEPARTURE_ONLY = GPIO_BUTTON_3; // Departures only mode
    constexpr int BUTTON_FACTORY_RESET = GPIO_BUTTON_1; // Long-press (5s) triggers factory reset
#endif

#if defined(BOARD_S3_E1001)
    // --- I2C Functional Names (SHT4x Indoor Sensor) ---
    constexpr int I2C_SDA = GPIO_I2C_SDA;
    constexpr int I2C_SCL = GPIO_I2C_SCL;
#endif
} // namespace Pins
