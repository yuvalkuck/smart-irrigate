//
// Created by uv on 16/08/2026.
//

#include "sensor_wind.h"
#include "logger.h"
static const char* TAG = "Wind:";

esp_err_t SensorWind::init(adc_oneshot_unit_handle_t master_bus_handler) {
    METHODTRACE
    return ESP_FAIL;
}
bool SensorWind::read(TelemetryData& data) {
    return false;
}
bool SensorWind::online(adc_oneshot_unit_handle_t bus) {
    METHODTRACE
    int raw_wind_sample = 0;
    if (adc_oneshot_read(bus, ADC_CHANNEL_3, &raw_wind_sample) == ESP_OK) {
        return true;
    } else {
        ESP_LOGE(TAG, "  [FAIL] ADC tracking core rejected wind speed sample processing.");
    }
    return false;
    
}
