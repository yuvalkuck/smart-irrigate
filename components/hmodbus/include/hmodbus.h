#ifndef HMODBUS_H
#define HMODBUS_H

#include <cstdint>
#include "esp_err.h"

// Modbus RTU master (esp-modbus) on the RS-485 link to the sensor cluster, 8N1.
#define HMODBUS_BAUD            9600
#define HMODBUS_TIMEOUT_MS      200
#define HMODBUS_MAX_REGISTERS   125

enum ModbusFunction : uint8_t {
    ReadHoldingRegisters = 0x03,
    ReadInputRegisters = 0x04,
};

esp_err_t init_modbus(void);

// Reads `count` registers into `out` (host byte order). Errors are the esp-modbus ones:
// ESP_ERR_TIMEOUT: no answer, ESP_ERR_INVALID_RESPONSE: bad frame, ESP_FAIL: slave exception.
esp_err_t modbus_read_registers(uint8_t slave, ModbusFunction function, uint16_t address,
                                uint16_t count, uint16_t* out);

#endif
