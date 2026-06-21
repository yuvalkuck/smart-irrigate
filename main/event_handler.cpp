#include <esp_log.h>
#include <hmqtt.h>
#include <protocol.h>
#include <common_event.h>
static const char* TAG = "AppEvent:";
ESP_EVENT_DEFINE_BASE(COMMON_BASE_EVENTS);

static void cbCommonEventHandler(void* handler_args, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    ESP_LOGV(TAG, "%s", __func__);
    char payload[64];
    if (event_base == COMMON_BASE_EVENTS) {
        EventData* event = static_cast<EventData*>(event_data);
        switch (event_id) {
            case COMMON_EVENT_SENSOR_UPDATED: {
                ESP_LOGV(TAG, "COMMON_EVENT_SENSOR_UPDATED");
                float *payload = static_cast<float*>(event->data);
                char send[64];
                // telemetryValues.temperature,    //!< temperature in degree C        (Invalid value -327.68)
                // telemetryValues.pressure,       //!< barometric pressure in hPascal (Invalid value 0.0)
                // telemetryValues.humidity,       //!< relative humidity in %         (Invalid value 0.0)
                // telemetryValues.gas_resistance, //!< gas resistance in Ohm
                snprintf(send, sizeof(send), "%.2f|%.2f|%.2f|%.2f", payload[0], payload[1],payload[2],payload[3]);
                mqtt_publish("/client/telemetry", send);
            }
            break;
            case COMMON_EVENT_ACCEPT_SERVER_CONFIGURATION: {
                ESP_LOGV(TAG, "COMMON_EVENT_ACCEPT_SERVER_CONFIGURATION");
                auto segment = static_cast<uint8_t*>(event->data);
                const std::vector<uint8_t> data(segment, segment + event->size);
                if (!setConfiguration(&data)) {
                    ESP_LOGE(TAG, "Failed to set configuration");
                }
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
