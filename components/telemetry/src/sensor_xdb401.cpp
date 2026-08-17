//
// Created by uv on 16/08/2026.
//

#include "sensor_xdb401.h"
#include "logger.h"
static const char* TAG = "XDB4xx:";
esp_err_t SensorXDB4xx::init(adc_oneshot_unit_handle_t master_bus_handler) {
    return ESP_FAIL;
}
bool SensorXDB4xx::read(TelemetryData& data) {
    return false;
}
bool SensorXDB4xx::online(adc_oneshot_unit_handle_t bus) {
    METHODTRACE
    int raw_adc_sample = 0;
    if (adc_oneshot_read(bus, ADC_CHANNEL_2, &raw_adc_sample) == ESP_OK) {
        if (raw_adc_sample > 150 && raw_adc_sample < 4000) {
            return true;
        } else {
            ESP_LOGE(TAG, "  [FAIL] XDB401 signal clipped out of bounds. Raw voltage fault: %d", raw_adc_sample);
        }
    } else {
        ESP_LOGE(TAG, "  [FAIL] ADC tracking core rejected sample processing.");
    }
    return false;
}
