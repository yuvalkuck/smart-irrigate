//
// Created by uv on 16/08/2026.
//

#ifndef SMART_IRRIGATE_SENSOR_CONCEPT_H
#define SMART_IRRIGATE_SENSOR_CONCEPT_H
#include <concepts>
#include "esp_err.h"
#include "driver/i2c_types.h"
#include "common_event.h"
#include "onewire_bus.h"
#include "esp_adc/adc_oneshot.h"

template <typename T>
concept SensorConcept = requires(T instance, TelemetryData& data) {
    { instance.read(data) } -> std::same_as<bool>;
};

template <typename T>
concept SensorI2C = requires(T instance, i2c_master_bus_handle_t bus) {
    { instance.init(bus) } -> std::same_as<esp_err_t>;
    { instance.online(bus) } -> std::same_as<bool>;
};

template <typename T>
concept SensorADC = requires(T instance, adc_oneshot_unit_handle_t bus) {
    { instance.init(bus) } -> std::same_as<esp_err_t>;
    { instance.online(bus) } -> std::same_as<bool>;
};

template <typename T>
concept SensorOneWire = requires(T instance, onewire_bus_handle_t bus) {
    { instance.init(bus) } -> std::same_as<esp_err_t>;
    { instance.online(bus) } -> std::same_as<bool>;
};

class AbstractSensor {
    protected:
       bool initialized_ = false;
};
template <typename Derived>
class AbstractSensorI2C : public AbstractSensor {
    public:
    explicit AbstractSensorI2C() {
        static_assert(SensorConcept<Derived>,
                      "The derived class does not implement the required SensorConcept methods!");
        static_assert(SensorI2C<Derived>,
                      "The derived class does not implement the required SensorI2C methods!");
    }
};
template <typename Derived>
class AbstractSensorADC : public AbstractSensor {
    public:
    explicit AbstractSensorADC() {
        static_assert(SensorConcept<Derived>,
                      "The derived class does not implement the required SensorConcept methods!");
        static_assert(SensorADC<Derived>,
                      "The derived class does not implement the required SensorI2C methods!");
    }
};


template <typename Derived>
class AbstractSensorOneWire : public AbstractSensor {
    public:
    explicit AbstractSensorOneWire() {
        static_assert(SensorConcept<Derived>,
                      "The derived class does not implement the required SensorConcept methods!");
        static_assert(SensorOneWire<Derived>,
                      "The derived class does not implement the required SensorI2C methods!");
    }
};

#endif //SMART_IRRIGATE_SENSOR_CONCEPT_H
