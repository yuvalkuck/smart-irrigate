#include <esp_log.h>
#include <hmqtt.h>
#include <protocol.h>
#include <common_event.h>
#include "flash.h"
static const char* TAG = "AppEvent:";
ESP_EVENT_DEFINE_BASE(COMMON_BASE_EVENTS);
static char uniqueName[33] = {0};
static void cbCommonEventHandler(void* handler_args, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    ESP_LOGV(TAG, "%s", __func__);
    if (event_base == COMMON_BASE_EVENTS) {
        time_t timestamp = time(nullptr);
        auto event = static_cast<EventData*>(event_data);
        switch (event_id) {
            case COMMON_EVENT_UPDATED_SENSOR: {
                ESP_LOGV(TAG, "COMMON_EVENT_UPDATED_SENSOR");
                auto payload = static_cast<float*>(event->data);
                char send[128];
                /** record:
                 * - unix timestamp in hex
                 * - station uniqeName
                 * - temperature C (Fix 1)
                 * - Groud Temperature C (Fix 1)
                 * - humidity % (Fix 1)
                 * - Presher (Fix 1)
                 * - Global radition (Fix 1)
                 **/
                snprintf(send, sizeof(send), "0x%llx|%s|%.1f|0.0|%.1f|0.0|0.0",timestamp,uniqueName, payload[0], payload[1]);
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
    NvsConfig nvs;
    nvs.getStr(CFG_NVS_KEY_DEVICE_UNIQUE,uniqueName,sizeof(uniqueName));
    if ( strlen(uniqueName) < 1 ) {
        memmove(uniqueName, "<NA>", 4);
    }
    esp_event_handler_instance_register(
        COMMON_BASE_EVENTS,
        ESP_EVENT_ANY_ID,
        &cbCommonEventHandler,
        NULL,
        NULL
    );
}
