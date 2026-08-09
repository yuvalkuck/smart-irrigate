//
// Created by uv on 06/08/2026.
//
#include "diagnostics.h"
#include "logger.h"
#include "FreeRTOSConfig.h"
#include "freertos/projdefs.h"
#include "portmacro.h"
#include "esp_adc/adc_oneshot.h"
#include "sht4x.h"
extern adc_oneshot_unit_handle_t adc_handle;
/**
 * @brief Synchronously validates individual sensor registers and electrical metrics.
 * @param i2c_bus Active master bus peripheral handle from the modern ESP-IDF 6.0 engine.
 * @param adc_handle Initialized handle targeting the 12-bit ADC1 hardware block.
 * @return uint8_t An 8-bit mask detailing operational integrity. Bits are marked '1' on success, '0' on fail.
 */
static const char* TAG = "Diagnostic:";
namespace Diagnostics {
    // Hex values for targeting the I2C physical layer addresses
    constexpr uint8_t BMP581_I2C_ADDR = 0x47;
    constexpr uint8_t TSL2591_I2C_ADDR = 0x29;

    constexpr MaskStateOK& operator|=(MaskStateOK& lhs, MaskStateOK rhs) {
        lhs = lhs | rhs;
        return lhs;
    }
    constexpr MaskStateOK operator|(MaskStateOK lhs, MaskStateOK rhs) {
        return static_cast<MaskStateOK>(
            static_cast<std::underlying_type_t<MaskStateOK>>(lhs) |
            static_cast<std::underlying_type_t<MaskStateOK>>(rhs)
        );
    }    

    MaskStateOK execute_operational_self_test(
        i2c_master_bus_handle_t i2c_bus
    ) noexcept {
        METHODTRACE
        MaskStateOK diagnostic_mask = MaskStateOK::Reset;

        // --- Step 1: Probe SHT41 ---
        if (i2c_master_probe(i2c_bus, I2C_SHT4X_DEV_ADDR_LO, pdMS_TO_TICKS(50)) == ESP_OK) {
            diagnostic_mask |= MaskStateOK::SHT41;
        }
        else {
            ESP_LOGE(TAG, "  [FAIL] SHT41 interface dropped communications.");
        }
#if defined(AVALIABLE_SENSOR)
        // --- Step 2: Probe BMP581 ---
        if (i2c_master_probe(i2c_bus, BMP581_I2C_ADDR, pdMS_TO_TICKS(50)) == ESP_OK) {
            diagnostic_mask |= MaskStateOK::BMP581;
        }
        else {
            ESP_LOGE(TAG, "  [FAIL] BMP581 interface dropped communications.");
        }

        // --- Step 3: Probe TSL2591 ---
        if (i2c_master_probe(i2c_bus, TSL2591_I2C_ADDR, pdMS_TO_TICKS(50)) == ESP_OK) {
            diagnostic_mask |= MaskStateOK::TSL2591;
        }
        else {
            ESP_LOGE(TAG, "  [FAIL] TSL2591 interface dropped communications.");
        }

        // --- Step 4: Probe DS18B20 ---
        // NOTE: Replace this placeholder statement directly with your internal 1-Wire hardware reset method call
        bool ds18b20_presence_detected = true;

        if (ds18b20_presence_detected) {
            diagnostic_mask |= MaskStateOK::DS18B20;
        }
        else {
            ESP_LOGE(TAG, "  [FAIL] DS18B20 missing or line ground short detected.");
        }
#endif

        // --- Step 5: Evaluate XDB401 Electrical Window ---
        int raw_adc_sample = 0;
        if (adc_oneshot_read(adc_handle, ADC_CHANNEL_2, &raw_adc_sample) == ESP_OK) {
            if (raw_adc_sample > 150 && raw_adc_sample < 4000) {
                diagnostic_mask |= MaskStateOK::XDB401;
            }
            else {
                ESP_LOGE(TAG, "  [FAIL] XDB401 signal clipped out of bounds. Raw voltage fault: %d", raw_adc_sample);
            }
        }
        else {
            ESP_LOGE(TAG, "  [FAIL] ADC tracking core rejected sample processing.");
        }
#if defined(AVALIABLE_SENSOR)
        // --- Step 6: Evaluate Analog Wind Speed Sensor Electrical Window ---
        int raw_wind_sample = 0;
        if (adc_oneshot_read(adc_handle, ADC_CHANNEL_3, &raw_wind_sample) == ESP_OK) {
            // Enforce boundary check to identify line shorts (close to 0) or floating/shorted inputs (close to max)
            if (raw_wind_sample > 50 && raw_wind_sample < 4050) {
                diagnostic_mask |= MaskStateOK::WindSpeed;
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
}