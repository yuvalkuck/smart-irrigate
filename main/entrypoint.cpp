#include <algorithm>
#include <telemetry.h>

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_event.h"
#include "flash.h"
#include "hwifi.h"
#include "hmqtt.h"
#include "hsntp.h"
#include "blesrv.h"
#include "driver/gpio.h"
#include "time.h"
#include "common_event.h"
#include "bme680.h"
#include "../../../esp/esp-idf/components/nvs_flash/include/nvs.h"

static const char* TAG = "App:";
extern bme680_values_float_t telemetryValues;
#define GPIO_LED 15
ESP_EVENT_DEFINE_BASE(COMMON_BASE_EVENTS);

static void setLedState(int fliper) {
    // ESP_LOGI(TAG, "set %d", fliper);
    gpio_set_level((gpio_num_t)GPIO_LED, fliper);
}

void continue_after_time_sync_cb(struct timeval* tv) {
    ESP_LOGI(TAG, "Notification of a time synchronization event");
    setenv("TZ", CONFIG_DEFAULT_LOCALE_TIME_ZONE, 1);
    tzset();
    ESP_LOGI(TAG, "set TimeZone to: %s", CONFIG_DEFAULT_LOCALE_TIME_ZONE);
    start_hmqtt();
}
static void cbCommonEventHandler(void* handler_args, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    ESP_LOGV(TAG, "%s", __func__);
    if (event_base == COMMON_BASE_EVENTS) {
        switch (event_id) {
            case COMMON_EVENT_SENSOR_UPDATED:
                ESP_LOGI(TAG, "Temp: %.2f °C | Humidity: %.2f %% | Pressure: %.2f hPa",
                         telemetryValues.temperature, telemetryValues.humidity, telemetryValues.pressure);

                ESP_LOGI(TAG, "Gas Resistance: %.2f Ohm", telemetryValues.gas_resistance);
                break;
            default:
                break;
        }
    }

}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "setting up");

    gpio_reset_pin((gpio_num_t)GPIO_LED);
    gpio_set_direction((gpio_num_t)GPIO_LED, GPIO_MODE_OUTPUT);

    //Initialize NVS
    esp_err_t ret = init_flash();
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "start event loop default");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // test if we have wifi config
    u_int32_t hCfg;
    if ( nvs_config(&hCfg) != ESP_OK) {
        return;
    }
    if ( !nvs_key_exist(&hCfg, CFG_NVS_KEY_WIFI_SSID) ) {
        init_blesrv();
        start_blesrv();
    } else {
        esp_event_handler_register(
                COMMON_BASE_EVENTS,          // Event base
                ESP_EVENT_ANY_ID,            // Event ID (or look for a specific one like CUSTOM_EVENT_SENSOR_READY)
                &cbCommonEventHandler,   // The callback function pointer
                NULL                         // Optional arguments passed to handler_args
            );
        init_telemetry();

        init_wifi();
        // // 6. Start Wi-Fi
        start_wifi();
        // SNTP
        init_sntp(CONFIG_NTP_SERVER, continue_after_time_sync_cb);
        init_hmqtt();
        start_sntp();
        start_telemetry();
    }
    nvs_close(hCfg);
    //
    ESP_LOGI(TAG, "working stage");
    //
    int ledFlip = 0;
    for (;;) {
        --ledFlip;
        if (ledFlip < 0) {
            ledFlip = 1;
        }
        setLedState(ledFlip);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    // if (CONFIG_LOG_MAXIMUM_LEVEL > CONFIG_LOG_DEFAULT_LEVEL) {
    //     /* If you only want to open more logs in the wifi module, you need to make the max level greater than the default level,
    //      * and call esp_log_level_set() before esp_wifi_init() to improve the log level of the wifi module. */
    //     esp_log_level_set("wifi", CONFIG_LOG_MAXIMUM_LEVEL);
    // }
}
