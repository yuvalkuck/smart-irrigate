#include <bme680.h>
#include <esp_log.h>
#include <hmqtt.h>
#include <protocol.h>
#include <common_event.h>
static const char* TAG = "AppEvent:";
extern bme680_values_float_t telemetryValues;
ESP_EVENT_DEFINE_BASE(COMMON_BASE_EVENTS);

static void cbCommonEventHandler(void* handler_args, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    ESP_LOGV(TAG, "%s", __func__);
    char payload[64];
    if (event_base == COMMON_BASE_EVENTS) {
        switch (event_id) {
            case COMMON_EVENT_SENSOR_UPDATED:
                snprintf(payload, sizeof(payload), "%.2f|%.2f|%.2f|%.2f", telemetryValues.temperature,
                         telemetryValues.humidity,
                         telemetryValues.pressure,
                         telemetryValues.gas_resistance);
                mqtt_publish("/client/telemetry", payload);
                break;
            case COMMON_EVENT_ACCEPT_SERVER_CONFIGURATION:
                if (!setConfiguration(static_cast<std::vector<uint8_t>*>(event_data))) {
                    ESP_LOGE(TAG, "Failed to set configuration");
                }
                break;
            default:
                break;
        }
    }
}

void init_event_app_handle() {
    esp_event_handler_instance_register(
        COMMON_BASE_EVENTS,
        ESP_EVENT_ANY_ID,
        &cbCommonEventHandler,
        NULL,
        NULL
    );
}
