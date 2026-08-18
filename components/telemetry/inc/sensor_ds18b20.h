//
// Created by uv on 16/08/2026.
//

#ifndef SMART_IRRIGATE_SENSOR_DS18B20_H
#define SMART_IRRIGATE_SENSOR_DS18B20_H
#include "sensor_concept.h"
class SensorDS18B20 : public AbstractSensorOneWire<SensorDS18B20> {
    public:
    esp_err_t init(onewire_bus_handle_t);
    bool read(TelemetryData& );
    bool online(onewire_bus_handle_t);
};

#endif //SMART_IRRIGATE_SENSOR_DS18B20_H
