// #include <stdio.h>
#include "wifi_softap.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "sdkconfig.h"
#include <lwip/ip4_addr.h>
#include <esp_http_server.h>
//#include "flash.h"

static const char* TAG = "wifi_softap:";
static httpd_handle_t server_instance = NULL;
//
// Created by uv on 12/07/2026.
//
#define DEFAULT_STATIC_SSID_NAME "Irrigate------------"

static void wifiEventHandlerSoftAP(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        esp_wifi_connect(); // Start connection once driver is ready
        server_instance = start_webserver();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        httpd_stop(server_instance);
        esp_wifi_connect(); // Retry connection if dropped
        ESP_LOGI(TAG, "Retrying connection to AP...");
    }
}



static void setAddressAP(esp_netif_t *ap_netif) {
    // 3. Stop the DHCP Server before modifying IP structures
        ESP_ERROR_CHECK(esp_netif_dhcps_stop(ap_netif));

    // 4. Define your custom network parameters
    esp_netif_ip_info_t ip_info;

    // Set your SoftAP IP Address (e.g., 192.168.10.1)
    ip4addr_aton(CONFIG_DEVICE_IP_DEFAULT_GATEWAY, (ip4_addr_t *)&ip_info.ip);
    // Set the Gateway IP (usually matches the AP's own IP address)
    ip4addr_aton(CONFIG_DEVICE_IP_DEFAULT_GATEWAY, (ip4_addr_t *)&ip_info.gw);
    // Set the Subnet Mask (e.g., 255.255.255.0)
    ip4addr_aton(CONFIG_DEVICE_IP_DEFAULT_MASK, (ip4_addr_t *)&ip_info.netmask);

    // 5. Apply the new configuration to the interface
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_netif, &ip_info));

    // 6. Restart the DHCP Server to hand out IPs in the new subnet range
    ESP_ERROR_CHECK(esp_netif_dhcps_start(ap_netif));

}

static void genDefaultSSID(char* buff) {
    // buff is minimum 32 chars
    uint8_t lenDefault = strlen(CONFIG_DEVICE_DEFAULT_UNIQUE_SSID);
    if (lenDefault > 0) {
        if (lenDefault > CONFIG_DEVICE_MAX_DEFAULT_UNIQUE_SSID) {
            lenDefault = CONFIG_DEVICE_MAX_DEFAULT_UNIQUE_SSID;
        }
        memmove(buff, CONFIG_DEVICE_DEFAULT_UNIQUE_SSID, lenDefault);
    }
    else {
        uint8_t mac[6] = {0};
        // Fetch the MAC address for the Wi-Fi Station interface
        esp_err_t ret = esp_read_mac(mac, ESP_MAC_WIFI_STA);

        if (ret == ESP_OK) {
            sprintf(buff, "Irrigate%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            ESP_LOGI(TAG, "DefaultSSID:%s", buff);
        }
        else {
            ESP_LOGW(TAG, "Failed to read MAC address, use hard coded");
            memmove(buff, DEFAULT_STATIC_SSID_NAME, sizeof(DEFAULT_STATIC_SSID_NAME));
        }
    }
}

void init_wifi_softap() {
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    ESP_ERROR_CHECK(esp_netif_init());
    setAddressAP(esp_netif_create_default_wifi_ap());

    // Wi-Fi Driver Initialization
    wifi_init_config_t wifiInitCfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifiInitCfg));
    // Register Event Handlers
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifiEventHandlerSoftAP, NULL, NULL));

    static char ssid[32] = {0};
    genDefaultSSID((char*)&ssid);
    wifi_config_t wifi_config = {
        .ap = {
            .channel = 0,
            .authmode = WIFI_AUTH_WPA2_WPA3_PSK,
            .max_connection = 1,
            .pmf_cfg = {
                .required = false,
            },
        },
    };
    memset(wifi_config.ap.password, 0, sizeof(wifi_config.ap.password));
    memmove(wifi_config.ap.password, CONFIG_DEVICE_DEFAULT_WPK_PASSWORD, sizeof(CONFIG_DEVICE_DEFAULT_WPK_PASSWORD));
    memmove(wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
    wifi_config.ap.ssid_len = strlen(ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
}
