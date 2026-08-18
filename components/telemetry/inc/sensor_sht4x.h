//
// Created by uv on 12/08/2026.
//

#ifndef SMART_IRRIGATE_SHT4X_SENSOR_H
#define SMART_IRRIGATE_SHT4X_SENSOR_H
#include "sensor_concept.h"
class SensorSHT4x : public AbstractSensorI2C<SensorSHT4x> {
    public:
    esp_err_t init(i2c_master_bus_handle_t);
    bool read(TelemetryData &);
    bool online(i2c_master_bus_handle_t);
};
#endif //SMART_IRRIGATE_SHT4X_SENSOR_H
