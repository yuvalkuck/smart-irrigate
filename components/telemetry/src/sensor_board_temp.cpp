#include "sensor_board_temp.h"
#include "logger.h"

static const char* TAG = "BoardTemp:";

// MCP9700A: 500 mV at 0 C, 10 mV per C
static constexpr float MilliVoltAtZero = 500.0f;
static constexpr float MilliVoltPerCelsius = 10.0f;

esp_err_t SensorBoardTemp::init(i2c_master_bus_handle_t bus) {
    METHODTRACE
    auto rc = adc_.init(bus);
    if (rc != ESP_OK) {
        return rc;
    }
    initialized_ = true;
    return ESP_OK;
}

bool SensorBoardTemp::read(TelemetryData& data) {
    if (!initialized_) {return false;}
    float millivolts = 0;
    auto rc = adc_.readMilliVolts(Ads1115::Channel::Ain2, millivolts);
    if (rc != ESP_OK) {
        ESP_LOGE(TAG, "board temperature read failed: %s", esp_err_to_name(rc));
        return false;
    }
    data.board_temperature = (millivolts - MilliVoltAtZero) / MilliVoltPerCelsius;
    LOGTRACE(TAG, "Board Temperature: %.2f °C (%.0f mV)", data.board_temperature, millivolts);
    return true;
}

bool SensorBoardTemp::online(i2c_master_bus_handle_t bus) {
    METHODTRACE
    return (i2c_master_probe(bus, Ads1115::AddressGnd, pdMS_TO_TICKS(200)) == ESP_OK);
}
