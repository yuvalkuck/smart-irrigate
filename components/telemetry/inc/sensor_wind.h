//
// Created by uv on 16/08/2026.
//

#ifndef SMART_IRRIGATE_SENSOR_WIND_H
#define SMART_IRRIGATE_SENSOR_WIND_H
#include "sensor_concept.h"
class SensorWind : public AbstractSensorADC<SensorWind> {
    public:
    esp_err_t init(adc_oneshot_unit_handle_t);
    bool read(TelemetryData &);
    bool online(adc_oneshot_unit_handle_t);
};

#endif //SMART_IRRIGATE_SENSOR_WIND_H
