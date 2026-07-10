#include "util/indoor_sensor.h"

#ifdef PCB_E1001

#include <Wire.h>
#include <SensirionI2cSht4x.h>
#include <esp_log.h>

static const char* TAG = "INDOOR_SENSOR";

// I2C pins for SHT4x on E1001 board
static constexpr uint8_t SHT4X_SDA = 19;
static constexpr uint8_t SHT4X_SCL = 20;
static constexpr uint8_t SHT4X_ADDR = 0x44;

static SensirionI2cSht4x sht4x;

// Static member initialization
bool IndoorSensor::initialized = false;
float IndoorSensor::temperature = 0.0f;
float IndoorSensor::humidity = 0.0f;

bool IndoorSensor::init() {
    Wire.begin(SHT4X_SDA, SHT4X_SCL);
    sht4x.begin(Wire, SHT4X_ADDR);

    uint32_t serialNumber = 0;
    uint16_t error = sht4x.serialNumber(serialNumber);

    if (error) {
        ESP_LOGE(TAG, "SHT4x not found (I2C error). Check wiring.");
        initialized = false;
        return false;
    }

    ESP_LOGI(TAG, "SHT4x initialized. Serial: %u", serialNumber);
    initialized = true;
    return true;
}

bool IndoorSensor::read() {
    if (!initialized) {
        ESP_LOGW(TAG, "Sensor not initialized, skipping read");
        return false;
    }

    uint16_t error = sht4x.measureHighPrecision(temperature, humidity);

    if (error) {
        ESP_LOGE(TAG, "SHT4x measurement failed (error: %u)", error);
        return false;
    }

    ESP_LOGI(TAG, "Indoor: %.1f°C, %.1f%% RH", temperature, humidity);
    return true;
}

float IndoorSensor::getTemperature() {
    return temperature;
}

float IndoorSensor::getHumidity() {
    return humidity;
}

bool IndoorSensor::isAvailable() {
    return initialized;
}

#endif // PCB_E1001
