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

extern "C" {
    static int8_t convertEspToBmp5(esp_err_t esp_error) {
        switch (esp_error) {
            case ESP_OK:
                return BMP5_OK;

            case ESP_ERR_INVALID_ARG:
                // Commonly maps to null pointer checks or improper structural inputs in sensor setups
                return BMP5_E_NULL_PTR;

            case ESP_ERR_TIMEOUT:
            case ESP_FAIL:
                // Maps communication bus timeouts (I2C/SPI) or absolute component transaction failures
                return BMP5_E_COM_FAIL;

            case ESP_ERR_NOT_FOUND:
                // Maps missing I2C addresses or structural components on the peripheral bus
                return BMP5_E_DEV_NOT_FOUND;

            case ESP_ERR_NOT_SUPPORTED:
                // Maps requests for unaligned configurations or power modes
                return BMP5_E_INVALID_POWERMODE;

            default:
                // Fallback for unhandled system or driver faults to a generic communication or initialization failure
                return BMP5_E_COM_FAIL;
        }
    }

    BMP5_INTF_RET_TYPE esp_bmp581_i2c_read(uint8_t reg_addr, uint8_t* reg_data, uint32_t length, void* intf_ptr) {
        if (!intf_ptr || !reg_data) {
            return BMP5_E_NULL_PTR;
        }
        // ESP_LOGD(TAG, "esp_bmp581_i2c_read reg 0x%02X called with length=%i", reg_addr,length);
        auto dev_handle = (i2c_master_dev_handle_t)intf_ptr;
        // ESP_LOGD(TAG, "read: wbuf=%p wsize=%u rbuf=%p rsize=%lu dev=%p",
        //              (void*)&reg_addr, 1, (void*)reg_data, length, (void*)dev_handle);
        esp_err_t err = i2c_master_transmit_receive(dev_handle, &reg_addr, 1, reg_data,
                                                    length, -1);
        if (err == ESP_OK) {
            return BMP5_OK;
        }
        ESP_LOGE(TAG, "i2c_master_transmit_receive failed: ESP_ERROR %i", err);
        return convertEspToBmp5(err);
    }

    // Callback to write data via ESP-IDF 6.0 master handle
    BMP5_INTF_RET_TYPE esp_bmp581_i2c_write(uint8_t reg_addr, const uint8_t* reg_data, uint32_t length,
                                            void* intf_ptr) {
        if (!intf_ptr || !reg_data) {
            return BMP5_E_NULL_PTR;
        }

        // ESP_LOGD(TAG, "esp_bmp581_i2c_write reg 0x%02X called with length=%i", reg_addr,length);
        // Prevent buffer overflows from unexpected data packages
        if ((length + 1) > BMP581_MAX_WRITE_LEN) {
            return BMP5_E_COM_FAIL;
        }
        // Allocate buffer directly on the stack frame
        uint8_t write_buf[BMP581_MAX_WRITE_LEN];
        // Pack target register address first, then payload bytes
        write_buf[0] = reg_addr;
        std::memmove(write_buf + 1, reg_data, length);

        // Execute synchronous, deterministic transmission
        esp_err_t err = i2c_master_transmit(static_cast<i2c_master_dev_handle_t>(intf_ptr), write_buf, length + 1, -1);
        if (err == ESP_OK) {
            return BMP5_OK;
        }
        ESP_LOGE(TAG, "i2c_master_transmit failed: ESP_ERROR %i", err);
        return convertEspToBmp5(err);
    }

    void esp_bmp581_delay_us(uint32_t period, void* intf_ptr) {
        // If the library asks for a tiny microsecond window, block the core precisely
        if (period < 5000) {
            period = 5000;
        }
        if (period < 10000) {
            esp_rom_delay_us(period);
        } else {
            // Fall back to FreeRTOS ticks for larger operational pauses to prevent watchdog triggers
            vTaskDelay(pdMS_TO_TICKS((period + 999) / 1000));
        }
    }
}

static i2c_master_dev_handle_t bmp581_handle;
static i2c_device_config_t bmp581_config = {
    .dev_addr_length = I2C_ADDR_BIT_LEN_7,
    .device_address = BMP5_I2C_ADDR_SEC,
    .scl_speed_hz = 100000, // 100 kHz Standard Mode
    .scl_wait_us = 0,
};
static bmp5_osr_odr_press_config osr_odr_cfg = {};

static bmp5_dev bmp5_sensor = {
    .read = esp_bmp581_i2c_read,
    .write = esp_bmp581_i2c_write,
    .delay_us = esp_bmp581_delay_us,
    .intf = BMP5_I2C_INTF,
};

esp_err_t SensorBMP5xx::init(i2c_master_bus_handle_t master_bus_handler) {
    METHODTRACE
    ESP_ERROR_CHECK(i2c_master_bus_add_device(master_bus_handler, &bmp581_config, &bmp581_handle));
    bmp5_sensor.intf_ptr = bmp581_handle;

    vTaskDelay(pdMS_TO_TICKS(10));
    // 4. Initialize BMP581 via Bosch API execution
    std::memset(&osr_odr_cfg, 0, sizeof(osr_odr_cfg));
    osr_odr_cfg.osr_t = BMP5_OVERSAMPLING_2X;
    osr_odr_cfg.osr_p = BMP5_OVERSAMPLING_16X;
    osr_odr_cfg.press_en = BMP5_ENABLE;
    osr_odr_cfg.odr = BMP5_ODR_50_HZ;
    auto rc = bmp5_soft_reset(&bmp5_sensor);
    if (rc != BMP5_OK) {
        ESP_LOGE(TAG, "bmp5_soft_reset failed: %i", rc);
        return rc;
    }
    vTaskDelay(pdMS_TO_TICKS(5));
    rc = bmp5_init(&bmp5_sensor);
    if (rc != BMP5_OK) {
        ESP_LOGE(TAG, "bmp5_init failed: %i", rc);
        return rc;
    }

    rc = bmp5_set_osr_odr_press_config(&osr_odr_cfg, &bmp5_sensor);
    if (rc != BMP5_OK) {
        ESP_LOGE(TAG, "bmp5_set_osr_odr_press_config failed");
        return rc;
    }
    rc = bmp5_set_power_mode(BMP5_POWERMODE_NORMAL, &bmp5_sensor);
    if (rc != BMP5_OK) {
        ESP_LOGE(TAG, "bmp5_set_power_mode failed");
    }
    return rc;
}

bool SensorBMP5xx::read(TelemetryData& data) {
    bmp5_sensor_data prob = {};
    auto rc = bmp5_get_sensor_data(&prob, &osr_odr_cfg, &bmp5_sensor);
    if (rc == BMP5_OK) {
        ESP_LOGI(TAG, "Pressure: %.2f Pa | Temp: %.2f °C\n", prob.pressure, prob.temperature);
        data.pressure = prob.pressure;
        return true;
    }
    return false;
}

bool SensorBMP5xx::online(i2c_master_bus_handle_t bus) {
    return (i2c_master_probe(bus, BMP5_I2C_ADDR_SEC, pdMS_TO_TICKS(50)) == ESP_OK);
}
