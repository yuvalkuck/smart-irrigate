//
// Created by uv on 12/07/2026.
//
#include "wifi_softap.h"
#include "flash.h"
#include <array>
#include <esp_http_server.h>
#include <logger.h>
#include <fmt/core.h>

static const char* TAG = "WebServer";
#include <esp_http_server.h>
static void register_routes(httpd_handle_t server);

httpd_handle_t start_webserver() {
    METHODTRACE
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Memory optimizations for ESP-IDF 6.x
    config.stack_size = 4096; // Down from 4096 (minimum safe stack)
    config.max_open_sockets = 2; // Down from 7 (limits concurrent clients)
    config.max_uri_handlers = 4; // Allocate only what you need
    config.recv_wait_timeout = 2; // Low timeout frees sockets faster
    config.send_wait_timeout = 2;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) == ESP_OK) {
        register_routes(server);
        // Register URI handlers here
        return server;
    }
    return NULL;
}

const static auto *noUserInterface = R"(<html><header/><body>NoUserInterface</body></html>)";
// 1. Define the GET Handler Function
static esp_err_t get_rootpage(httpd_req_t* req) {
    METHODTRACE
    httpd_resp_send(req, noUserInterface, sizeof(noUserInterface));
    return ESP_OK;
}
static esp_err_t get_command_restart(httpd_req_t* req) {
    METHODTRACE
    esp_restart();
    return ESP_OK;
}


#define MAX_STRING_LENGTH 128

static esp_err_t get_status_handler(httpd_req_t* req) {
    METHODTRACE
    std::array<char[MAX_STRING_LENGTH], 5> strCache = {};
    NvsConfig hCfg;
    hCfg.getStr(CFG_NVS_KEY_WIFI_SSID, strCache[0],MAX_STRING_LENGTH);
    hCfg.getStr(CFG_NVS_KEY_WIFI_PASSWORD, strCache[1],MAX_STRING_LENGTH);
    hCfg.getStr(CFG_NVS_KEY_MQTT_URL, strCache[2],MAX_STRING_LENGTH);
    hCfg.getStr(CFG_NVS_KEY_MQTT_USERNAME, strCache[3],MAX_STRING_LENGTH);
    hCfg.getStr(CFG_NVS_KEY_MQTT_PASSWORD, strCache[4],MAX_STRING_LENGTH);
    char payload[1024] = {0};
    fmt::format_to_n(payload, sizeof(payload), R"({{"{}"="{}","{}"="{}","{}"="{}","{}"="{}","{}"="{}"}})",
                     CFG_NVS_KEY_WIFI_SSID, strCache[0],
                     CFG_NVS_KEY_WIFI_PASSWORD, strCache[1],
                     CFG_NVS_KEY_MQTT_URL, strCache[2],
                     CFG_NVS_KEY_MQTT_USERNAME, strCache[3],
                     CFG_NVS_KEY_MQTT_PASSWORD, strCache[4]);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, payload, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// 2. Define the POST Handler Function
static esp_err_t post_handler(httpd_req_t* req) {
    METHODTRACE
    char buf[256]; // Keep small to save stack memory
    int ret, remaining = req->content_len;
    // interface is mostly send config key and process it
    while (remaining > 0) {
        // Read the incoming data stream safely
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) continue;
            return ESP_FAIL;
        }
        remaining -= ret;
        auto key = buf;
        auto value = strchr(buf, '=');

        *value = 0; value++;
        ESP_LOGI(TAG, "accept: %s=%s", key, value);
        NvsConfig hCfg;
        hCfg.setStr(key, value);
    }

    httpd_resp_send_chunk(req, NULL, 0); // End response
    return ESP_OK;
}


// 3. Registering the Handlers inside your server initialization
static void register_routes(httpd_handle_t server) {
    METHODTRACE
    // GET Route Configuration
    {
        httpd_uri_t get_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = get_rootpage,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &get_uri);
    }
    {
        httpd_uri_t get_uri = {
            .uri = "/api/restart",
            .method = HTTP_GET,
            .handler = get_command_restart,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &get_uri);
    }

    httpd_uri_t get_uri = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = get_status_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &get_uri);

    // POST Route Configuration
    httpd_uri_t post_uri = {
        .uri = "/api/set",
        .method = HTTP_POST,
        .handler = post_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &post_uri);
}
