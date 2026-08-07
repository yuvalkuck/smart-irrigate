//
// Created by uv on 06/08/2026.
//
#include "diagnostics.h"
#include "esp_log.h"
#include "FreeRTOSConfig.h"
#include "portmacro.h"
#include "freertos/projdefs.h"
/**
 * @brief Synchronously validates individual sensor registers and electrical metrics.
 * @param i2c_bus Active master bus peripheral handle from the modern ESP-IDF 6.0 engine.
 * @param adc_handle Initialized handle targeting the 12-bit ADC1 hardware block.
 * @return uint8_t An 8-bit mask detailing operational integrity. Bits are marked '1' on success, '0' on fail.
 */
static const char* TAG = "Diagnostic:";

uint8_t execute_operational_self_test(
    i2c_master_bus_handle_t i2c_bus,
    adc_oneshot_unit_handle_t adc_handle
) noexcept {
    uint8_t diagnostic_mask = 0x00;

    // --- Step 1: Probe SHT41 ---
    if (i2c_master_probe(i2c_bus, SHT41_I2C_ADDR, pdMS_TO_TICKS(50)) == ESP_OK) {
        diagnostic_mask |= BIT_SHT41_OK;
    }
    else {
        ESP_LOGE(TAG, "  [FAIL] SHT41 interface dropped communications.");
    }
#if defined(AVALIABLE_SENSOR)
    // --- Step 2: Probe BMP581 ---
    if (i2c_master_probe(i2c_bus, BMP581_I2C_ADDR, pdMS_TO_TICKS(50)) == ESP_OK) {
        diagnostic_mask |= BIT_BMP581_OK;
    }
    else {
        ESP_LOGE(TAG, "  [FAIL] BMP581 interface dropped communications.");
    }

    // --- Step 3: Probe TSL2591 ---
    if (i2c_master_probe(i2c_bus, TSL2591_I2C_ADDR, pdMS_TO_TICKS(50)) == ESP_OK) {
        diagnostic_mask |= BIT_TSL2591_OK;
    }
    else {
        ESP_LOGE(TAG, "  [FAIL] TSL2591 interface dropped communications.");
    }

    // --- Step 4: Probe DS18B20 ---
    // NOTE: Replace this placeholder statement directly with your internal 1-Wire hardware reset method call
    bool ds18b20_presence_detected = true;

    if (ds18b20_presence_detected) {
        diagnostic_mask |= BIT_DS18B20_OK;
    }
    else {
        ESP_LOGE(TAG, "  [FAIL] DS18B20 missing or line ground short detected.");
    }

    // --- Step 5: Evaluate XDB401 Electrical Window ---
    int raw_adc_sample = 0;
    if (adc_oneshot_read(adc_handle, ADC_CHANNEL_0, &raw_adc_sample) == ESP_OK) {
        if (raw_adc_sample > 150 && raw_adc_sample < 4000) {
            diagnostic_mask |= BIT_XDB401_OK;
        }
        else {
            ESP_LOGE(TAG, "  [FAIL] XDB401 signal clipped out of bounds. Raw voltage fault: %d", raw_adc_sample);
        }
    }
    else {
        ESP_LOGE(TAG, "  [FAIL] ADC tracking core rejected sample processing.");
    }

    // --- Step 6: Evaluate Analog Wind Speed Sensor Electrical Window ---
    int raw_wind_sample = 0;
    if (adc_oneshot_read(adc_handle, ADC_CHANNEL_1, &raw_wind_sample) == ESP_OK) {
        // Enforce boundary check to identify line shorts (close to 0) or floating/shorted inputs (close to max)
        if (raw_wind_sample > 50 && raw_wind_sample < 4050) {
            diagnostic_mask |= BIT_WIND_OK;
        }
        else {
            ESP_LOGE(TAG, "  [FAIL] Wind Sensor signal clipped out of bounds. Raw voltage fault: %d", raw_wind_sample);
        }
    }
    else {
        ESP_LOGE(TAG, "  [FAIL] ADC tracking core rejected wind speed sample processing.");
    }
    #endif

    ESP_LOGI(TAG, "Result: 0x%02X", diagnostic_mask);

    return diagnostic_mask;
}
