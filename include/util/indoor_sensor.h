#pragma once

#ifdef PCB_E1001

/**
 * @brief Indoor temperature & humidity sensor (SHT4x) driver.
 *
 * The SHT4x is connected via I2C on the E1001 board:
 *   SDA = GPIO19, SCL = GPIO20, Address = 0x44
 *
 * Usage:
 *   IndoorSensor::init();
 *   if (IndoorSensor::read()) {
 *       float temp = IndoorSensor::getTemperature();
 *       float hum  = IndoorSensor::getHumidity();
 *   }
 */
class IndoorSensor {
public:
    static bool init();
    static bool read();
    static float getTemperature();
    static float getHumidity();
    static bool isAvailable();

private:
    static bool initialized;
    static float temperature;
    static float humidity;
};

#endif // PCB_E1001
