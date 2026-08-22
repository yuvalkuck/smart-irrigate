//
// Created by uv on 16/08/2026.
//
// #include "tsl2591.h"
#include "sensor_tsl2591.h"
#include "logger.h"
static const char* TAG = "TSL25xx:";
#define I2C_MASTER_FREQ_HZ  400000

/**
 * if "external master bus" ownership matters to you, this driver is a bad fit.
 *
 * The correct way is by AI is to:
 * Bypass esp-idf-lib entirely for this sensor and talk to the TSL2591 registers directly with
 * the plain ESP-IDF driver/i2c_master.h API (i2c_master_bus_add_device() against your own externally-created bus,
 * then i2c_master_transmit/i2c_master_transmit_receive).
 * This gives you true external-bus control at the cost of writing the ~10 register calls yourself (enable, control, channel reads, lux formula).
 *
 *  I will left the sensor to the end :(
 */
esp_err_t SensorTSL25xx::init(i2c_master_bus_handle_t master_bus_handler) {
    return ESP_FAIL;
}
bool SensorTSL25xx::read(TelemetryData& data) {
    // if (!initialized_) {return false;}
    return false;
}
bool SensorTSL25xx::online(i2c_master_bus_handle_t bus) {
    return false;
    // return (i2c_master_probe(bus, TSL2591_I2C_ADDR, pdMS_TO_TICKS(50)) == ESP_OK);
}

