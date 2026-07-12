#include <stdio.h>
#include "hwifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "sdkconfig.h"
#include <algorithm>
#include <time.h>

#include "flash.h"



static EventGroupHandle_t wifiEventGroup;
static const char* TAG = "hwifi:";

static void wifiEventHandlerSTA(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data) {
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
static void wifiEventHandlerSoftAP(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data) {
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

#define DEFAULT_STATIC_SSID_NAME "Irrigate------------"
static void genDefaultSSID(char *buff) {
    // buff is minimum 32 chars
    uint8_t lenDefault = strlen(CONFIG_DEVICE_DEFAULT_UNIQUE_SSID);
    if ( lenDefault > 0) {
        if ( lenDefault > CONFIG_DEVICE_MAX_DEFAULT_UNIQUE_SSID) {
            lenDefault = CONFIG_DEVICE_MAX_DEFAULT_UNIQUE_SSID;
        }
        memmove(buff, CONFIG_DEVICE_DEFAULT_UNIQUE_SSID, lenDefault);
    } else {
        uint8_t mac[6] = {0};
        // Fetch the MAC address for the Wi-Fi Station interface
        esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);

        if (ret == ESP_OK) {
            sprintf(buff, "Irrigate%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            ESP_LOGI(TAG, "DefaultSSID:%s",buff);

        } else {
            ESP_LOGW(TAG, "Failed to read MAC address, use hard coded");
            memmove(buff, DEFAULT_STATIC_SSID_NAME, sizeof(DEFAULT_STATIC_SSID_NAME));
        }
    }
}
void init_wifi_softap() {
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_ap();
    // Wi-Fi Driver Initialization
    wifi_init_config_t wifiInitCfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifiInitCfg));
    // event group must be decalre because use int the event handler
    wifiEventGroup = xEventGroupCreate();
    // Register Event Handlers

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandlerSoftAP, NULL, NULL));
    static char ssid[32] = {0};
    genDefaultSSID((char *)&ssid);
    wifi_config_t wifi_config = {
        .ap = {
            .channel = 0,
            .authmode = WIFI_AUTH_OPEN,
            .max_connection = 1,
            .pmf_cfg = {
                .required = false,
            },
        },
    };
    memmove(wifi_config.ap.ssid, ssid,sizeof(wifi_config.ap.ssid));
    wifi_config.ap.ssid_len = strlen(ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));

}
void init_wifi_sta() {
    // Network Interface & Event Loop Initialization
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    ESP_ERROR_CHECK(esp_netif_init());

    esp_netif_create_default_wifi_sta();

    // Wi-Fi Driver Initialization
    wifi_init_config_t wifiInitCfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifiInitCfg));

    // event group must be decalre because use int the event handler
    wifiEventGroup = xEventGroupCreate();
    // Register Event Handlers

    // MQTT_EVENT_ANY = -1
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandlerSTA, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandlerSTA, NULL, NULL));

    // 5. Configure Station Mode
    NvsConfig hCfg;
    wifi_config_t wifi_config = {};
    hCfg.getStr( CFG_NVS_KEY_WIFI_SSID, wifi_config.sta.ssid, sizeof(wifi_config.sta.ssid));
    hCfg.getStr( CFG_NVS_KEY_WIFI_PASSWORD, wifi_config.sta.password, sizeof(wifi_config.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
}

void start_wifi() {
    // 6. Start Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_start());

    // Wait until connection is established
    xEventGroupWaitBits(wifiEventGroup, BIT0, pdFALSE, pdFALSE, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi setup complete.");
}

