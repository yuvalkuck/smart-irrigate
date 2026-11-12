#include "ads1115.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "Ads1115:";

static constexpr uint8_t RegConversion = 0x00;
static constexpr uint8_t RegConfig = 0x01;

// config register fields
static constexpr uint16_t ConfigOsStart = 1 << 15;      // write 1: start a single conversion, read 1: idle
static constexpr unsigned ConfigMuxShift = 12;          // MUX[2:0]
static constexpr uint16_t ConfigPga2V048 = 0b010 << 9;  // +-2.048 V
static constexpr uint16_t ConfigModeSingle = 1 << 8;    // single-shot
static constexpr uint16_t ConfigRate128 = 0b100 << 5;   // 128 SPS
static constexpr uint16_t ConfigComparatorOff = 0b00011;

static constexpr int BusTimeoutMs = 100;
static constexpr int ConversionTimeoutMs = 50; // a conversion takes about 8 ms at 128 SPS
static constexpr float MilliVoltPerCount = 2048.0f / 32768.0f;

esp_err_t Ads1115::init(i2c_master_bus_handle_t bus, uint8_t address) {
    esp_err_t err = i2c_master_probe(bus, address, BusTimeoutMs);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "no device at 0x%02X: %s", address, esp_err_to_name(err));
        return err;
    }
    i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = 400000,
    };
    err = i2c_master_bus_add_device(bus, &config, &dev_);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "add device 0x%02X failed: %s", address, esp_err_to_name(err));
        dev_ = nullptr;
    }
    return err;
}

esp_err_t Ads1115::readMilliVolts(Channel channel, float& millivolts) {
    if (dev_ == nullptr) {
        return ESP_ERR_INVALID_STATE;
    }
    const uint16_t config = ConfigOsStart | (static_cast<uint16_t>(channel) << ConfigMuxShift) |
                            ConfigPga2V048 | ConfigModeSingle | ConfigRate128 | ConfigComparatorOff;
    const uint8_t start[3] = {RegConfig, static_cast<uint8_t>(config >> 8), static_cast<uint8_t>(config & 0xFF)};
    esp_err_t err = i2c_master_transmit(dev_, start, sizeof(start), BusTimeoutMs);
    if (err != ESP_OK) {
        return err;
    }

    // wait until the OS bit reads 1 again
    const uint8_t configRegister = RegConfig;
    uint8_t status[2] = {};
    bool done = false;
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(ConversionTimeoutMs);
    while (!done && xTaskGetTickCount() < deadline) {
        vTaskDelay(1);
        err = i2c_master_transmit_receive(dev_, &configRegister, 1, status, sizeof(status), BusTimeoutMs);
        if (err != ESP_OK) {
            return err;
        }
        done = (status[0] & (ConfigOsStart >> 8)) != 0;
    }
    if (!done) {
        return ESP_ERR_TIMEOUT;
    }

    const uint8_t conversionRegister = RegConversion;
    uint8_t data[2] = {};
    err = i2c_master_transmit_receive(dev_, &conversionRegister, 1, data, sizeof(data), BusTimeoutMs);
    if (err != ESP_OK) {
        return err;
    }
    const int16_t raw = static_cast<int16_t>(data[0] << 8 | data[1]);
    millivolts = raw * MilliVoltPerCount;
    return ESP_OK;
}
