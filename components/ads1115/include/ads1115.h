#ifndef ADS1115_H
#define ADS1115_H

#include <cstdint>
#include "esp_err.h"
#include "driver/i2c_master.h"

// ADS1115 16-bit ADC on an existing I2C master bus (the shared bus of the board manager).
// Single-shot, single-ended reads at +-2.048 V full scale, 128 SPS.
class Ads1115 {
    i2c_master_dev_handle_t dev_ = nullptr;
    public:
    static constexpr uint8_t AddressGnd = 0x48; // ADDR pin on GND

    // value is the MUX field of the config register: input against GND
    enum class Channel : uint8_t { Ain0 = 4, Ain1 = 5, Ain2 = 6, Ain3 = 7 };

    esp_err_t init(i2c_master_bus_handle_t bus, uint8_t address = AddressGnd);
    // Starts a conversion on `channel`, waits for it and returns the voltage at the input in mV.
    esp_err_t readMilliVolts(Channel channel, float& millivolts);
};

#endif // ADS1115_H
