#ifndef COMMON_EVENT_H
#define COMMON_EVENT_H

#include "esp_event.h"


ESP_EVENT_DECLARE_BASE(COMMON_BASE_EVENTS);

enum {
    COMMON_EVENT_SENSOR_UPDATED,
    COMMON_EVENT_ACCEPT_SERVER_CONFIGURATION
};

struct EventData {
    size_t size;

    void * data;
    size_t length() const {return size+sizeof(EventData);}
};

struct TelemetryData {
    float temperature;    //!< temperature in degree C
    float humidity;       //!< relative humidity in %
};

#endif