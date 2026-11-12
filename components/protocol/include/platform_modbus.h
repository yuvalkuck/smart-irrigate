#ifndef PLATFORM_MODBUS_H
#define PLATFORM_MODBUS_H
#include <cstdint>

// Modbus map of the sensor platform (platform/sensor), shared by the C6 master and the platform firmware.
// Slave PlatformModbus::Slave, input registers (function 0x04), all 16-bit.
// The start address of a read is a request code (Register) and the count is ResponseCount() of that code,
// so one quantity or a group can be read at once. Slave behaviour: platform/sensor/README.md.
namespace PlatformModbus {
    constexpr uint8_t Slave = 1;

    // quantities, the address of each one
    enum Register : uint16_t {
        Invalid      = 0,
        Temperature  = 1, // int16, 0.01 C             (SHT41)
        Humidity     = 2, // uint16, 0.01 %RH          (SHT41)
        HumidityTemp = 3, // 2 registers: humidity (0.01 %RH), then temperature (int16, 0.01 C)
        Solar        = 4, // uint16, mV at the PA0 pin (solar panel)
        Pressure     = 5, // uint16, 10 Pa             (BMP581)
        Wind         = 6, // planned, not answered yet
        Status       = 7, // uint16, bit mask of Fault, 0 = all sensors of the platform work
        AllAvaliable = 0xFF // reserved, not answered yet
    };

    // answer of the Status request: a bit is set for each platform sensor that failed
    enum Fault : uint16_t {
        FaultNone  = 0,
        FaultSHT   = 1 << 0,
        FaultBMP   = 1 << 1,
        FaultSolar = 1 << 2,
    };

    // number of registers to read, from the address of the first quantity of the group
    constexpr uint16_t CountTemperature = 1;
    constexpr uint16_t CountHumidity = 1;
    constexpr uint16_t CountTempHumidity = 2; // from Temperature
    constexpr uint16_t CountPressure = 1;
    constexpr uint16_t CountSolar = 1;
    constexpr uint16_t CountAll = 4; // from Temperature

    // number of registers the slave answers for a request (0 = not a readable request)
    constexpr uint16_t ResponseCount(Register request) {
        switch (request) {
            case HumidityTemp:                                  return 2; // humidity, temperature
            case Temperature: case Humidity: case Solar:
            case Pressure: case Status:                         return 1;
            default:                                            return 0;
        }
    }
}

#endif // PLATFORM_MODBUS_H
