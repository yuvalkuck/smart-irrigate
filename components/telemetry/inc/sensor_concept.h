//
// Created by uv on 16/08/2026.
//

#ifndef SMART_IRRIGATE_SENSOR_CONCEPT_H
#define SMART_IRRIGATE_SENSOR_CONCEPT_H
#include <concepts>
#include "esp_err.h"
#include "driver/i2c_types.h"
#include "common_event.h"
template <typename T>
concept SensorConcept = requires(T instance, i2c_master_bus_handle_t bus, TelemetryData& data) {
    { instance.init(bus) } -> std::same_as<esp_err_t>;
    { instance.read(data) } -> std::same_as<bool>;
    { instance.online(bus) } -> std::same_as<bool>;
};

template <typename Derived>
class SensorAbstract {
    public:
    explicit SensorAbstract() {
        static_assert(SensorConcept<Derived>,
                      "The derived class does not implement the required SensorConcept methods!");
    }
};

#endif //SMART_IRRIGATE_SENSOR_CONCEPT_H
