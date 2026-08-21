//
// Created by uv on 16/08/2026.
//

#include "sensor_xdb401.h"
#include "logger.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define XDB401_ADC_CHANNEL   ADC_CHANNEL_2
#define XDB401_ADC_ATTEN     ADC_ATTEN_DB_12
#define XDB401_ADC_BITWIDTH  ADC_BITWIDTH_DEFAULT

static adc_cali_handle_t dev_handle = nullptr;
static adc_oneshot_unit_handle_t bus_handle = nullptr;

static const char* TAG = "XDB4xx:";

esp_err_t SensorXDB4xx::init(adc_oneshot_unit_handle_t bus) {
    METHODTRACE
    bus_handle = bus;
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = XDB401_ADC_ATTEN,
        .bitwidth = XDB401_ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(bus, XDB401_ADC_CHANNEL, &chan_cfg));
    adc_cali_curve_fitting_config_t dev_config = {
        .unit_id = ADC_UNIT_1,
        .chan = XDB401_ADC_CHANNEL,
        .atten = XDB401_ADC_ATTEN,
        .bitwidth = XDB401_ADC_BITWIDTH,
    };
    auto rc = adc_cali_create_scheme_curve_fitting(&dev_config, &dev_handle);
    if (rc != ESP_OK) {
        return rc;
    }
    initialized_ = true;
    return ESP_OK;
}

// XDB401 typical transfer function (check your datasheet variant!):
// Vout = Vsupply * (0.1 * P/Pmax + 0.5)   ->  ratiometric to 5V or 3.3V supply
// Rearranged for Vsupply = 3300 mV, Pmax = your sensor's full-scale range (e.g. 100 kPa):

// Voltage divider: Vsensor -- R1(1.8k) -- [ADC node] -- R2(3k) -- GND
#define XDB401_DIV_R1_OHM     1800.0f
#define XDB401_DIV_R2_OHM     3000.0f

// Sensor transfer function (5V supply, 0-1 MPa range)
#define XDB401_V_MIN_MV       500.0f   // 0.5V = 0 MPa
#define XDB401_V_MAX_MV       4500.0f  // 4.5V = 1 MPa
#define XDB401_P_MAX_MPA      1.0f

// ...

const constexpr float divider_ratio = XDB401_DIV_R2_OHM / (XDB401_DIV_R1_OHM + XDB401_DIV_R2_OHM); // = 0.625

bool SensorXDB4xx::read(TelemetryData& data) {
    if (!initialized_) {return false;}
    int raw_value = 0;
    if (adc_oneshot_read(bus_handle, XDB401_ADC_CHANNEL, &raw_value) != ESP_OK) {
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
    if (sensor_mv < XDB401_V_MIN_MV) { sensor_mv = XDB401_V_MIN_MV; }
    if (sensor_mv > XDB401_V_MAX_MV) { sensor_mv = XDB401_V_MAX_MV; }

    // Convert sensor voltage to pressure
    data.water_pressure = ((sensor_mv - XDB401_V_MIN_MV) / (XDB401_V_MAX_MV - XDB401_V_MIN_MV)) * XDB401_P_MAX_MPA;
    ESP_LOGI(TAG, "Water Pressure: %.02f",data.water_pressure);
    return true;
}

bool SensorXDB4xx::online(adc_oneshot_unit_handle_t bus) {
    METHODTRACE
    int raw_adc_sample = 0;
    if (adc_oneshot_read(bus, ADC_CHANNEL_2, &raw_adc_sample) == ESP_OK) {
        return true;
    } else {
        ESP_LOGE(TAG, "ADC tracking core rejected sample processing.");
    }
    return false;
}
