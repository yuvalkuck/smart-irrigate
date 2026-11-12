#ifndef SMART_IRRIGATE_SENSOR_PLATFORM_H
#define SMART_IRRIGATE_SENSOR_PLATFORM_H
#include "sensor_concept.h"
#include "platform_modbus.h"

// Remote sensor cluster (platform/sensor) on the RS-485 Modbus RTU link: SHT41, BMP581 and solar level.
// The requests and their answers are in platform_modbus.h.
constexpr int PlatformSteps = 3; // SHT41, BMP581, solar

class SensorPlatform : public AbstractSensor {
    PlatformModbus::Register requests_[PlatformSteps] = {}; // what to ask the platform, built once in init()
    int count_ = 0; // requests_[0 .. count_-1] are set, they run from count_-1 down to 0
    public:
    // Reads the ENABLE_HW mask once and builds the request list. A mask change needs a restart.
    esp_err_t init();
    bool read(TelemetryData&);
};

static_assert(SensorConcept<SensorPlatform>, "SensorPlatform does not implement SensorConcept!");

#endif //SMART_IRRIGATE_SENSOR_PLATFORM_H
