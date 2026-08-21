#include <esp_log.h>
#include <hmqtt.h>
#include <protocol.h>
#include <common_event.h>
#include "flash.h"
#include <array>
static constexpr auto TAG = "AppEvent:";
ESP_EVENT_DEFINE_BASE(COMMON_BASE_EVENTS);
static std::array<char, 33> uniqueName;

static void cbCommonEventHandler(void* handler_args, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    ESP_LOGV(TAG, "%s", __func__);
    if (event_base == COMMON_BASE_EVENTS) {
        auto event = static_cast<EventData*>(event_data);
        switch (event_id) {
            case COMMON_EVENT_UPDATED_SENSOR: {
                ESP_LOGV(TAG, "COMMON_EVENT_UPDATED_SENSOR");
                auto payload = static_cast<TelemetryData*>(event->data);
                char send[128];
                /** record:
                 * - unix timestamp in hex
                 * - station uniqeName
                 * - temperature C (Fix 1)
                 * - Groud Temperature C (Fix 1)
                 * - humidity % (Fix 1)
                 * - Presher (Fix 1)
                 * - Global radition (Fix 1)
                 * - Water line pressure MPa (Fix 1)
                 **/
                snprintf(send, sizeof(send), "0x%llx|%s|%.1f|%.1f|%.1f|%.1f|%.1f|%.1f|%.1f",
                         time(nullptr), uniqueName.data(),
                         payload->air_temperature,
                         payload->soile_temperature,
                         payload->humidity,
                         payload->pressure,
                         payload->solar_level,
                         payload->wind_speed,
                         payload->water_pressure
                );
                mqtt_publish("/client/telemetry", send);
            }
            break;
            case COMMON_EVENT_ACCEPT_SERVER_CONFIGURATION: {
                ESP_LOGV(TAG, "COMMON_EVENT_ACCEPT_SERVER_CONFIGURATION");
                if (!setConfiguration(static_cast<const char*>(event->data), event->size)) {
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
    nvs.getStr(CFG_NVS_KEY_DEVICE_UNIQUE, uniqueName.data(), uniqueName.size());
    if (uniqueName[0] == 0) {
        memmove(uniqueName.data(), "<NA>", 4);
        memset(uniqueName.data() + 4, 0, uniqueName.size() - 4);
    }
    esp_event_handler_instance_register(
        COMMON_BASE_EVENTS,
        ESP_EVENT_ANY_ID,
        &cbCommonEventHandler,
        nullptr,
        nullptr
    );
}
