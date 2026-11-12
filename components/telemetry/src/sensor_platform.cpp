#include "sensor_platform.h"
#include "hmodbus.h"
#include "flash.h"
#include "logger.h"

static const char* TAG = "Platform:";
using namespace PlatformModbus;

static constexpr int MaxResponse = 2; // registers of the longest answer, see ResponseCount()

static void decode(Register request, const uint16_t* reg, TelemetryData& data) {
    switch (request) {
        case HumidityTemp:
            data.air_humidity = reg[0] / 100.0f;
            data.air_temperature = static_cast<int16_t>(reg[1]) / 100.0f;
            break;
        case Pressure:
            data.air_pressure = reg[0] * 10.0f; // Pa, as the on-board BMP581
            break;
        case Solar:
            data.solar_level = reg[0]; // relative level (mV at the pin), not calibrated yet
            break;
        default:
            break;
    }
}

// asks the platform which of its sensors failed
static void logFaults() {
    uint16_t faults;
    if (modbus_read_registers(Slave, ReadInputRegisters, Status, ResponseCount(Status), &faults) == ESP_OK) {
        ESP_LOGW(TAG, "platform faults: 0x%x", faults);
    }
}

esp_err_t SensorPlatform::init() {
    METHODTRACE
    count_ = 0;
    NvsConfig cfg;
    uint16_t mask = 0;
    if (!cfg || !cfg.getMask(SETUP_KEY(ENABLE_HW), mask)) {
        ESP_LOGE(TAG, "failed to read the hardware enable mask");
        return ESP_FAIL;
    }
    if (!(mask & static_cast<uint16_t>(HardwareEnable::PlatformSensor))) {
        ESP_LOGI(TAG, "sensor platform disabled");
        return ESP_OK;
    }
    if (mask & static_cast<uint16_t>(HardwareEnable::SensorSHT)) requests_[count_++] = HumidityTemp;
    if (mask & static_cast<uint16_t>(HardwareEnable::SensorBME)) requests_[count_++] = Pressure;
    if (mask & static_cast<uint16_t>(HardwareEnable::SensorSolar)) requests_[count_++] = Solar;
    ESP_LOGI(TAG, "sensor platform enabled, %d requests", count_);
    return ESP_OK;
}

bool SensorPlatform::read(TelemetryData& data) {
    bool ok = false;
    for (int i = count_ - 1; i >= 0; --i) {
        const Register request = requests_[i];
        uint16_t reg[MaxResponse];
        const auto rc = modbus_read_registers(Slave, ReadInputRegisters, request, ResponseCount(request), reg);
        if (rc != ESP_OK) {
            ESP_LOGW(TAG, "request %d failed: %s", request, esp_err_to_name(rc));
            if (rc != ESP_ERR_TIMEOUT) logFaults(); // no answer at all: asking again only waits again
            continue;
        }
        decode(request, reg, data);
        ok = true;
    }
    return ok;
}
