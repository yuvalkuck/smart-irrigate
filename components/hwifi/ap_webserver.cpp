//
// Created by uv on 12/07/2026.
//
#include "wifi_softap.h"
#include "flash.h"

#include <esp_http_server.h>
#include <esp_log.h>
#include <fmt/core.h>

static const char *TAG = "WebServer";
static char workBuf[128] = {0};
static char payload[1024] = {0};
#include <esp_http_server.h>

httpd_handle_t start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Memory optimizations for ESP-IDF 6.x
    config.stack_size = 3072;          // Down from 4096 (minimum safe stack)
    config.max_open_sockets = 2;       // Down from 7 (limits concurrent clients)
    config.max_uri_handlers = 4;       // Allocate only what you need
    config.recv_wait_timeout = 2;      // Low timeout frees sockets faster
    config.send_wait_timeout = 2;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        // Register URI handlers here
        return server;
    }
    return NULL;
}

// 1. Define the GET Handler Function
esp_err_t get_handler(httpd_req_t *req) {
    NvsConfig hCfg;


    /*
    hCfg.getStr( CFG_NVS_KEY_WIFI_SSID, workBuf, sizeof(workBuf));
    const char *resp_str = "Hello From ESP32";
    static constexpr const char* CFG_NVS_KEY_WIFI_SSID = "wifi_ssid";
    static constexpr const char* CFG_NVS_KEY_WIFI_PASSWORD = "wifi_password";
    static constexpr const char* CFG_NVS_KEY_BT_DEVICE_NAME = "device_name";
    static constexpr const char* CFG_NVS_KEY_MQTT_URL = "mqtt_url";
    static constexpr const char* CFG_NVS_KEY_MQTT_USERNAME = "mqtt_uname";
    static constexpr const char* CFG_NVS_KEY_MQTT_PASSWORD = "mqtt_pword";
    static constexpr const char* CFG_NVS_KEY_NTP_SERVER = "ntp_server";
    static constexpr const char* CFG_NVS_KEY_LOCALE_TZ = "locale_tz";
    */

    //httpd_resp_send(req, resp_str, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// 2. Define the POST Handler Function
esp_err_t post_handler(httpd_req_t *req) {
    char buf[64]; // Keep small to save stack memory
    int ret, remaining = req->content_len;

    while (remaining > 0) {
        // Read the incoming data stream safely
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) continue;
            return ESP_FAIL;
        }
        remaining -= ret;
        // Process 'buf' chunk here if needed
    }

    httpd_resp_send_chunk(req, NULL, 0); // End response
    return ESP_OK;
}

// 3. Registering the Handlers inside your server initialization
void register_routes(httpd_handle_t server) {
    // GET Route Configuration
    httpd_uri_t get_uri = {
        .uri      = "/api/data",
        .method   = HTTP_GET,
        .handler  = get_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &get_uri);

    // POST Route Configuration
    httpd_uri_t post_uri = {
        .uri      = "/api/submit",
        .method   = HTTP_POST,
        .handler  = post_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &post_uri);
}
