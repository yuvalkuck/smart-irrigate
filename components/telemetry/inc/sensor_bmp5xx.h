//
// Created by uv on 12/08/2026.
//

#ifndef SMART_IRRIGATE_BMP5XX_SENSOR_H
#define SMART_IRRIGATE_BMP5XX_SENSOR_H
#include "esp_err.h"
#include "driver/i2c_types.h"
struct TelemetryData;
class SensorBMP5xx {
public:
    SensorBMP5xx() = default;
    esp_err_t init(i2c_master_bus_handle_t);
    bool read(TelemetryData &);
};
#endif //SMART_IRRIGATE_BMP5XX_SENSOR_H
