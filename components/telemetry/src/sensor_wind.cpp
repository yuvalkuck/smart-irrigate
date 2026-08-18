//
// Created by uv on 16/08/2026.
//

#include "sensor_wind.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "logger.h"
static const char* TAG = "Wind:";
#define WIND_ADC_CHANNEL     ADC_CHANNEL_3
#define WIND_ADC_ATTEN       ADC_ATTEN_DB_12
#define WIND_ADC_BITWIDTH    ADC_BITWIDTH_DEFAULT

static adc_cali_handle_t dev_handle = nullptr;
static adc_oneshot_unit_handle_t bus_handle = nullptr;

esp_err_t SensorWind::init(adc_oneshot_unit_handle_t bus) {
    bus_handle = bus;
    METHODTRACE
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = WIND_ADC_ATTEN,
        .bitwidth = WIND_ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(bus, WIND_ADC_CHANNEL, &chan_cfg));
    adc_cali_curve_fitting_config_t dev_config = {
        .unit_id = ADC_UNIT_1,
        .chan = WIND_ADC_CHANNEL,
        .atten = WIND_ADC_ATTEN,
        .bitwidth = WIND_ADC_BITWIDTH,
    };
    auto rc = adc_cali_create_scheme_curve_fitting(&dev_config, &dev_handle);
    if (rc != ESP_OK) {
        return rc;
    }
    initialized_ = true;
    return ESP_OK;
}

// Wind sensor transfer function:
// Vout = 0-5V linear, mapped to 0-30 m/s
// Voltage divider: Vsensor -- R1(1.8k) -- [ADC node] -- R2(3k) -- GND
#define WIND_DIV_R1_OHM      1800.0f
#define WIND_DIV_R2_OHM      3000.0f

#define WIND_V_MIN_MV        0.0f      // 0V = 0 m/s
#define WIND_V_MAX_MV        5000.0f   // 5V = full scale
#define WIND_SPEED_MAX_MS    30.0f

const constexpr float divider_ratio = WIND_DIV_R2_OHM / (WIND_DIV_R1_OHM + WIND_DIV_R2_OHM); // = 0.625
bool SensorWind::read(TelemetryData& data) {
    if (!initialized_) {return false;}
    int raw_value = 0;
    if (adc_oneshot_read(bus_handle, WIND_ADC_CHANNEL, &raw_value) != ESP_OK) {
        ESP_LOGE(TAG, "ADC read failed");
        return false;
    }

    int voltage_mv = 0;
    if (adc_cali_raw_to_voltage(dev_handle, raw_value, &voltage_mv) != ESP_OK) {
        ESP_LOGE(TAG, "ADC calibration conversion failed");
        return false;
    }

    float sensor_mv = (float)voltage_mv / divider_ratio;

    // Clamp to valid sensor output range (protects against noise pushing slightly out of bounds)
    if (sensor_mv < WIND_V_MIN_MV) { sensor_mv = WIND_V_MIN_MV; }
    if (sensor_mv > WIND_V_MAX_MV) { sensor_mv = WIND_V_MAX_MV; }

    data.wind_speed = ((sensor_mv - WIND_V_MIN_MV) / (WIND_V_MAX_MV - WIND_V_MIN_MV)) * WIND_SPEED_MAX_MS;
    ESP_LOGI(TAG, "Wind Speed:%.02f",data.wind_speed);
    return true;
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
