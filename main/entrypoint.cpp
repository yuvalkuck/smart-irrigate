#include <algorithm>
#include <protocol.h>
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


static const char* TAG = "App:";
#define GPIO_LED GPIO_NUM_15
#define GPIO_FORCE_BLE GPIO_NUM_17

void init_event_app_handle();
static void setLedState(int fliper) {
    // ESP_LOGI(TAG, "set %d", fliper);
    gpio_set_level((gpio_num_t)GPIO_LED, fliper);
}

void continue_after_time_sync_cb(struct timeval* tv) {
    ESP_LOGI(TAG, "Notification of a time synchronization event");
    NvsConfig hCfg;
    char buff[128] = {0};
    if (hCfg.getStr(CFG_NVS_KEY_LOCALE_TZ, buff, sizeof(buff))) {
        setenv("TZ", buff, 1);
        tzset();
        ESP_LOGI(TAG, "set TimeZone to: %s", buff);
        start_hmqtt();
    }
    else {
        ESP_LOGE(TAG, "failed to set timezone from fkasg");
    }
}



static void init_gpio() {
    gpio_reset_pin((gpio_num_t)GPIO_LED);
    gpio_set_direction((gpio_num_t)GPIO_LED, GPIO_MODE_OUTPUT);
    gpio_reset_pin((gpio_num_t)GPIO_FORCE_BLE);
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_FORCE_BLE),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    vTaskDelay(pdMS_TO_TICKS(100));
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "setting up");
    ESP_LOGI(TAG, "free heap: %iK", esp_get_free_heap_size()/1024);

    init_gpio();
    //Initialize NVS
    bool initRegular = (gpio_get_level(GPIO_FORCE_BLE) == 0 ? false : true);
    esp_err_t ret = init_flash();

    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "start event loop default");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    setLedState(true);
    // test if we have wifi config
    { // block to release hCfg when not use anymore

        NvsConfig hCfg;
        if (!hCfg) {
            ESP_LOGE(TAG, "failed to init config flash");
            return;
        }
        if (initRegular || hCfg.isStrEmpty(CFG_NVS_KEY_WIFI_SSID)) {
            ESP_LOGI(TAG, "Start BLE");
            init_blesrv();
            start_blesrv();
            initRegular = false;
            ESP_LOGI(TAG, "free heap: %iK", esp_get_free_heap_size()/1024);
        } else {
            ESP_LOGI(TAG, "Start Regular Load");
            init_event_app_handle();
            init_telemetry();

            init_wifi();
            // // 6. Start Wi-Fi
            start_wifi();
            // SNTP
            char buff[128] = {0};
            if (hCfg.getStr(CFG_NVS_KEY_NTP_SERVER, buff, sizeof(buff))) {
                init_sntp(buff, continue_after_time_sync_cb);
            }
            else {
                ESP_LOGE(TAG, "failed to get NTP server url from flash");
            }
            // MQTT
            init_hmqtt();
            start_sntp();
            start_telemetry();
        }
    }
    ESP_LOGI(TAG, "working stage");
    auto ticker = (initRegular ? 2000 : 250);
    int ledFlip = 1;
    for (;;) {
        --ledFlip;
        if (ledFlip < 0) {
            ledFlip = 1;
        }
        setLedState(ledFlip);
        vTaskDelay(ticker / portTICK_PERIOD_MS);
    }
}
