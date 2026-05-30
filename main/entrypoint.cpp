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

static const char* TAG = "irrigate app";
// static EventGroupHandle_t wifiEventGroup;
#define GPIO_LED 15

static void setLedState(int fliper) {
    // ESP_LOGI(TAG, "set %d", fliper);
    gpio_set_level((gpio_num_t)GPIO_LED, fliper);
}

void continue_after_time_sync_cb(struct timeval* tv) {
    ESP_LOGI(TAG, "Notification of a time synchronization event");
    setenv("TZ", CONFIG_DEFAULT_LOCALE_TIME_ZONE, 1);
    tzset();
    ESP_LOGI(TAG,"set TimeZone to: %s",CONFIG_DEFAULT_LOCALE_TIME_ZONE);
    start_hmqtt();

}


extern "C" void app_main(void) {
    ESP_LOGI(TAG, "setting up");

    gpio_reset_pin((gpio_num_t)GPIO_LED);
    gpio_set_direction((gpio_num_t)GPIO_LED, GPIO_MODE_OUTPUT);

    //Initialize NVS
    esp_err_t ret  = init_flash();
    ESP_ERROR_CHECK(ret);
    //init_blesrv();

    ESP_LOGI(TAG, "start event loop default");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    //start_blesrv();
    init_telemetry();
#if 0
    init_wifi();
    // // 6. Start Wi-Fi
    start_wifi();
    // SNTP
    init_sntp(CONFIG_NTP_SERVER, continue_after_time_sync_cb);
    init_hmqtt();
    start_sntp();
    //
#endif

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
