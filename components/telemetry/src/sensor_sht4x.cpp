#include "sensor_sht4x.h"
#include "sht4x.h"
#include "logger.h"
static sht4x_handle_t sht41_handle{};
static sht4x_config_t sht41_config = {
    .i2c_address = I2C_SHT4X_DEV_ADDR_LO,
    .i2c_clock_speed = 100000,
    .repeat_mode = SHT4X_REPEAT_HIGH,
    .heater_mode = SHT4X_HEATER_OFF,
};
static const char* TAG = "SHT4x:";

esp_err_t SensorSHT4x::init(i2c_master_bus_handle_t master_bus_handler) {
    METHODTRACE
    auto rc = sht4x_init(master_bus_handler, &sht41_config, &sht41_handle);
    if (rc != ESP_OK) {
        ESP_ERROR_CHECK(rc);
        return rc;
    }
    return ESP_OK;
}

bool SensorSHT4x::read(TelemetryData& data) {
    auto res = sht4x_get_measurement(sht41_handle, &data.air_temperature, &data.humidity);
    if (res == ESP_OK) {
        ESP_LOGI(TAG, "Temperature: %.2f °C | Humidity: %.2f %%", data.air_temperature,
                 data.humidity);
        return true;
    }
    return false;
}

bool SensorSHT4x::online(i2c_master_bus_handle_t bus) {
    METHODTRACE
    return (i2c_master_probe(bus, I2C_SHT4X_DEV_ADDR_LO, pdMS_TO_TICKS(50)) == ESP_OK);
}
