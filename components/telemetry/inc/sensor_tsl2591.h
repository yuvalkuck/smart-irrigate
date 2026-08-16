//
// Created by uv on 16/08/2026.
//

#ifndef SMART_IRRIGATE_SENSOR_TSL2591_H
#define SMART_IRRIGATE_SENSOR_TSL2591_H
#include "sensor_concept.h"
class SensorTSL25xx : public AbstractSensorI2C<SensorTSL25xx> {
    public:
    esp_err_t init(i2c_master_bus_handle_t);
    bool read(TelemetryData& );
    bool online(i2c_master_bus_handle_t);
};

#endif //SMART_IRRIGATE_SENSOR_TSL2591_H
