#ifndef SMART_IRRIGATE_SENSOR_BOARD_TEMP_H
#define SMART_IRRIGATE_SENSOR_BOARD_TEMP_H
#include "sensor_concept.h"
#include "ads1115.h"

// Main board temperature: MCP9700A (U7) on ADS1115 AIN2.
class SensorBoardTemp : public AbstractSensorI2C<SensorBoardTemp> {
    Ads1115 adc_;
    public:
    esp_err_t init(i2c_master_bus_handle_t);
    bool read(TelemetryData &);
    bool online(i2c_master_bus_handle_t);
};

#endif //SMART_IRRIGATE_SENSOR_BOARD_TEMP_H
