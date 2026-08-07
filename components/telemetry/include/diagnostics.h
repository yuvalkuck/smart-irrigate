//
// Created by uv on 06/08/2026.
//

#ifndef IRRIGATE_DIAGNOSTICS_H
#define IRRIGATE_DIAGNOSTICS_H
#pragma once

#include "driver/i2c_master.h"
#include "esp_adc/adc_oneshot.h"

// ====================================================================
// HARDWARE DIAGNOSTIC BITMASK POSITIONS (C++17 Binary Assignments)
// ====================================================================
inline constexpr uint8_t BIT_SHT41_OK   = (1 << 0); // 0x01: Ambient Temp & Humidity
inline constexpr uint8_t BIT_BMP581_OK  = (1 << 1); // 0x02: Barometric Pressure
inline constexpr uint8_t BIT_TSL2591_OK = (1 << 2); // 0x04: Solar Irradiance
inline constexpr uint8_t BIT_DS18B20_OK = (1 << 3); // 0x08: Topsoil Subsurface Thermal
inline constexpr uint8_t BIT_XDB401_OK  = (1 << 4); // 0x10: Hydraulic Line Pressure
inline constexpr uint8_t BIT_WIND_OK    = (1 << 5);

// Hex values for targeting the I2C physical layer addresses
constexpr uint8_t SHT41_I2C_ADDR  = 0x44;
constexpr uint8_t BMP581_I2C_ADDR = 0x47;
constexpr uint8_t TSL2591_I2C_ADDR = 0x29;
// ... other bit assignments ...

// Declaration only: Tells the compiler this function exists globally
uint8_t execute_operational_self_test(i2c_master_bus_handle_t i2c_bus, adc_oneshot_unit_handle_t adc_handle) noexcept;

#endif //IRRIGATE_DIAGNOSTICS_H
