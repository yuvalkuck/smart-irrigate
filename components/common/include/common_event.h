#ifndef COMMON_EVENT_H
#define COMMON_EVENT_H

#include "esp_event.h"


ESP_EVENT_DECLARE_BASE(COMMON_BASE_EVENTS);

enum {
    COMMON_EVENT_UPDATED_SENSOR,
    COMMON_EVENT_UPDATED_POWER,
    COMMON_EVENT_ACCEPT_SERVER_CONFIGURATION
};

struct EventData {
    size_t size;
    void * data;
    size_t length() const {return size+sizeof(EventData);}
};

struct TelemetryData {
    float air_temperature;
    float air_humidity;
    float air_pressure;
    float water_pressure;
    float soile_temperature;
    float solar_level;
    float wind_speed;
    float board_temperature;
};

enum class PowerState : uint8_t {
    None             = 0x00,
    OnBattery        = 0x01,
    BatteryLow       = 0x02,
    Charging         = 0x04,
    ConverterEnabled = 0x08,
    S3Powered        = 0x10
};

struct PowerStatus {
    uint8_t state;            // mask of PowerState
    float bus_voltage;        // V, INA226 bus voltage on the 24 V bus
    float battery_current;    // A, INA226 via the shunt R31, charging positive
    float battery_power;      // W, bus_voltage * battery_current
    float battery_charge;     // Ah, coulomb counting
};
#endif