#include <stdio.h>
#include "hwifi.h"
#include "logger.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "sdkconfig.h"
#include "flash.h"

static EventGroupHandle_t wifiEventGroup;
static const char* TAG = "hwifi_sta:";

static void wifiEventHandlerSTA(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) {
    METHODTRACE
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect(); // Start connection once driver is ready
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect(); // Retry connection if dropped
        ESP_LOGI(TAG, "Retrying connection to AP...");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifiEventGroup, BIT0);
    }
}

void init_wifi_sta() {
    // Network Interface & Event Loop Initialization
    METHODTRACE
    ESP_ERROR_CHECK(esp_netif_init());

    esp_netif_create_default_wifi_sta();

    // Wi-Fi Driver Initialization
    wifi_init_config_t wifiInitCfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifiInitCfg));

    // Register Event Handlers

    // MQTT_EVENT_ANY = -1
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandlerSTA, NULL, NULL));
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandlerSTA, NULL, NULL));

    // 5. Configure Station Mode
    NvsConfig hCfg;
    wifi_config_t wifi_config = {};
    hCfg.getStr(CFG_NVS_KEY_WIFI_SSID, wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid));
    hCfg.getStr(CFG_NVS_KEY_WIFI_PASSWORD, wifi_config.sta.password, sizeof(wifi_config.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
}

void start_wifi() {
    METHODTRACE
    // 6. Start Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_start());
    // event group must be decalre because use int the event handler
    wifiEventGroup = xEventGroupCreate();
    configASSERT(wifiEventGroup);

    // Wait until connection is established
    xEventGroupWaitBits(wifiEventGroup, BIT0, pdFALSE, pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi setup complete.");
}
