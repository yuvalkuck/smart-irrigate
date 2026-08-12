#include "sensor_bmp5xx.h"

#include <cstring>
#include "logger.h"
#include "bmp5.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "common_event.h"

static const char* TAG = "BMP5xx:";
#define BMP581_MAX_WRITE_LEN 32 // AI estimate was half ...

BMP5_INTF_RET_TYPE esp_bmp581_i2c_read(uint8_t reg_addr, uint8_t* reg_data, uint32_t length, void* intf_ptr) {
    if (!intf_ptr || !reg_data) {
        return BMP5_E_NULL_PTR;
    }
    // ESP-IDF 6.0 combined write-read transaction
    esp_err_t err = i2c_master_transmit_receive(static_cast<i2c_master_dev_handle_t>(intf_ptr), &reg_addr, 1, reg_data,
                                                length, -1);
    if (err != ESP_OK) {
        return BMP5_E_COM_FAIL;
    }
    return BMP5_OK;
}

// Callback to write data via ESP-IDF 6.0 master handle
BMP5_INTF_RET_TYPE esp_bmp581_i2c_write(uint8_t reg_addr, const uint8_t* reg_data, uint32_t length, void* intf_ptr) {
    if (!intf_ptr || !reg_data) {
        return BMP5_E_NULL_PTR;
    }
    // Prevent buffer overflows from unexpected data packages
    if ((length + 1) > BMP581_MAX_WRITE_LEN) {
        return BMP5_E_COM_FAIL;
    }
    // Allocate buffer directly on the stack frame
    uint8_t write_buf[BMP581_MAX_WRITE_LEN];
    // Pack target register address first, then payload bytes
    write_buf[0] = reg_addr;
    memmove(&write_buf[1], reg_data, length);
    // Execute synchronous, deterministic transmission
    esp_err_t err = i2c_master_transmit(static_cast<i2c_master_dev_handle_t>(intf_ptr), write_buf, length + 1, -1);
    if (err != ESP_OK) {
        return BMP5_E_COM_FAIL;
    }
    return BMP5_OK;
}

void esp_bmp581_delay_us(uint32_t period, void *intf_ptr)
{
    // Convert microseconds to FreeRTOS ticks, ensuring minimum 1 tick duration
    vTaskDelay(pdMS_TO_TICKS((period + 999) / 1000));
}
static i2c_master_dev_handle_t bmp581_handle;
static i2c_device_config_t bmp581_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = BMP5_I2C_ADDR_SEC,
    .scl_speed_hz = 100000, // 100 kHz Standard Mode
};
static bmp5_osr_odr_press_config osr_odr_cfg = {
    .osr_t = BMP5_OVERSAMPLING_2X, // use recommended value
    .osr_p = BMP5_OVERSAMPLING_16X, // use recommended value
    .press_en = BMP5_ENABLE,
    .odr = BMP5_ODR_50_HZ
};

static bmp5_dev bmp5_sensor = {
    .intf_ptr = bmp581_handle,
    .read = esp_bmp581_i2c_read,
    .write = esp_bmp581_i2c_write,
    .delay_us = esp_bmp581_delay_us,
    .intf = BMP5_I2C_INTF,
}; // Pass device handle as interface pointer

esp_err_t SensorBMP5xx::init(i2c_master_bus_handle_t master_bus_handler) {
    METHODTRACE
    ESP_ERROR_CHECK(i2c_master_bus_add_device(master_bus_handler, &bmp581_config, &bmp581_handle));
    // 3. Link Bosch API structure to ESP-IDF driver context

    // 4. Initialize BMP581 via Bosch API execution
    auto rc  = bmp5_init(&bmp5_sensor);
    if (rc != BMP5_OK) {
        ESP_LOGE(TAG, "bmp5_init failed");
        return rc;
    }

    rc = bmp5_set_osr_odr_press_config(&osr_odr_cfg, &bmp5_sensor);
    if (rc != BMP5_OK) {
        ESP_LOGE(TAG, "bmp5_set_osr_odr_press_config failed");
        return rc;
    }
    rc =  bmp5_set_power_mode(BMP5_POWERMODE_FORCED, &bmp5_sensor);
    if (rc != BMP5_OK) {
        ESP_LOGE(TAG, "bmp5_set_power_mode failed");
    }
        return rc;
}

bool SensorBMP5xx::read(TelemetryData&data) {
    bmp5_sensor_data prob;
        auto rc = bmp5_get_sensor_data(&prob,&osr_odr_cfg, &bmp5_sensor);
        if (rc == BMP5_OK) {
            ESP_LOGI(TAG, "Pressure: %.2f Pa | Temp: %.2f °C\n", prob.pressure, prob.temperature);
            data.pressure = prob.pressure;
            return true;
        }
    return false;

}
