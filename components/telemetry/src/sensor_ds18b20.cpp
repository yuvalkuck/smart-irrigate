//
// Created by uv on 16/08/2026.
//

#include "sensor_ds18b20.h"
#include "ds18b20.h"
#include "logger.h"
static constexpr auto TAG = "DS18B20:";
extern onewire_bus_handle_t onewire_bus_handle;
static ds18b20_device_handle_t dev_handle = nullptr;
#define DS18B20_FAMILY_CODE 0x28

esp_err_t SensorDS18B20::init(onewire_bus_handle_t bus) {
    METHODTRACE
    // find device configuration by scanning current connected devices
    constexpr int kMaxScanRetries = 5;
    onewire_device_t dev;
    bool found = false;
    esp_err_t rc = ESP_OK;
    for (int attempt = 1; !found && attempt <= kMaxScanRetries; attempt++) {
        onewire_device_iter_handle_t iter = nullptr;
        ESP_ERROR_CHECK(onewire_new_device_iter(bus, &iter));
        // ESP_ERR_NOT_FOUND is how the driver signals "search complete", not a fault.
        // ESP_ERR_INVALID_CRC means bus noise corrupted a bit mid-search (the long
        // 1-Wire run is noise-prone, see GPIO_ONEWIRE_BUS comment) -- retry the scan
        // rather than treating it the same as "no device present".
        while ((rc = onewire_device_iter_get_next(iter, &dev)) == ESP_OK) {
            LOGTRACE(TAG, "device add: 0x%016" PRIX64, dev.address);
            if ((uint8_t)(dev.address & 0xFF) == DS18B20_FAMILY_CODE) {
                found = true;
                break;
            }
        }
        onewire_del_device_iter(iter);

        if (rc == ESP_ERR_INVALID_CRC) {
            ESP_LOGW(TAG, "ROM search CRC error (attempt %d/%d), retrying", attempt, kMaxScanRetries);
        }
    }

    if (!found) {
        ESP_LOGE(TAG, "No DS18B20 found on bus");
        return ESP_ERR_NOT_FOUND;
    }

    ds18b20_config_t ds_cfg = {}; // defaults are fine
    esp_err_t err = ds18b20_new_device_from_bus(bus, &ds_cfg, &dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ds18b20_new_device() failed: %s", esp_err_to_name(err));
        return err;
    }

    // Optional: set resolution (9-12 bit, default is usually 12-bit)
    ds18b20_set_resolution(dev_handle, DS18B20_RESOLUTION_12B);

    LOGTRACE(TAG, "initialized, ROM 0x%016" PRIX64, dev.address);
    initialized_ = true;
    return ESP_OK;
}

bool SensorDS18B20::read(TelemetryData& data) {
    if (!initialized_) { return false; }
    esp_err_t err = ds18b20_trigger_temperature_conversion(dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "conversion trigger failed: %s", esp_err_to_name(err));
        return false;
    }

    err = ds18b20_get_temperature(dev_handle, &data.soile_temperature);
    if (err == ESP_OK) {
        LOGTRACE(TAG, "Soil Temperature:%.02f",data.soile_temperature);
        return true;
    }
    ESP_LOGE(TAG, "soil temperature read failed: %s", esp_err_to_name(err));
    return false;
}

bool SensorDS18B20::online(onewire_bus_handle_t bus) {
    METHODTRACE
    auto err = onewire_bus_reset(bus);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "DS18B20 online (presence pulse detected)");
        return true;
    }

    if (err == ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "DS18B20 offline -- no presence pulse. Check power, "
                 "wiring, and pull-up resistor.");
    } else {
        ESP_LOGE(TAG, "onewire_bus_reset() error: %s", esp_err_to_name(err));
    }
    return false;
}
