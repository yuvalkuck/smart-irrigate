#include "esp_log.h"
#include "flash.h"

#include <algorithm>

#include "sdkconfig.h"
#include "nvs_flash.h"

#define NVS_NAMESPACE "."
#define NVS_PARTITION_NAME "config"

static const char* TAG = "flash:";
static nvs_handle_t hNVS;

bool NvsConfig::isStrEmpty(const char* key) const {
    char tmp[128] = {0};
    size_t len = sizeof(tmp);
    auto err = nvs_get_str(handler_, key, tmp, &len);
    switch (err) {
        case ESP_ERR_NVS_INVALID_HANDLE:
            ESP_LOGE(TAG, "ESP_ERR_NVS_INVALID_HANDLE");
            return true;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGI(TAG, "ESP_ERR_NVS_NOT_FOUND");
            return true;
        case ESP_ERR_NVS_INVALID_LENGTH:
            ESP_LOGE(TAG, "ESP_ERR_NVS_INVALID_LENGTH");
            return false;
        case ESP_OK:
            if (strlen(tmp) < 1) {
                return true;
            }
            break;
        default:
            ESP_LOGE(TAG, "Unhandled error: %d", err);
    }
    return false;

}

static void nvsCreateKeyStr(const char* name, const char* defaule_value = "") {
    ESP_LOGI(TAG, "%s:%s", __func__, name);
    esp_err_t err;

    char tmp[1] = {0};
    size_t len = sizeof(tmp);
    // Step 1: Get the size of the stored string (includes null terminator)
    err = nvs_get_str(hNVS, name, tmp, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        err = nvs_set_str(hNVS, name, defaule_value);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(TAG, "failed to create: %s", defaule_value);
        }
    }
}

static esp_err_t nvs_config(nvs_handle_t* handler) {
    esp_err_t ret = nvs_open_from_partition(NVS_PARTITION_NAME,NVS_NAMESPACE, NVS_READWRITE, handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed (%d) to open partition: %s:%s", ret, NVS_PARTITION_NAME, NVS_NAMESPACE);
    }
    return ret;
}

NvsConfig::NvsConfig() {
    nvs_config(&handler_);
}

NvsConfig::~NvsConfig() {
    nvs_close(handler_);
}

bool NvsConfig::getStr(const char* key, char* value, size_t len) {
    char buffer[128] = {};
    size_t cpylen = sizeof(buffer);
    if (nvs_get_str(handler_, key, buffer, &cpylen) != ESP_OK) {
        return false;
    }
    memcpy(value, buffer, std::min(len, cpylen));
    return true;

}

// bool nvs_get_value_str(const nvs_handle_t handler, const char *key, char *value, size_t len,const char *defval) {
//     nvs_get_str(handler, key, value, &len);
//     if ( defval)
//     size_t trglen = std::min(
// }
// size_t ssid_len = std::min(sizeof(wifi_config.sta.ssid), strlen(WIFI_SSID));
// memcpy(wifi_config.sta.ssid, WIFI_SSID, ssid_len);

esp_err_t init_flash(void) {
    ESP_LOGI(TAG, "nvs flash init");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_flash_init();
    }
    ret = nvs_flash_init_partition(NVS_PARTITION_NAME);
    if (ret != ESP_OK) {
        ESP_ERROR_CHECK(nvs_flash_erase_partition(NVS_PARTITION_NAME));
        nvs_flash_init_partition(NVS_PARTITION_NAME);
    }
    ret = nvs_config(&hNVS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed (%d) to open partition: %s:%s", ret, NVS_PARTITION_NAME, NVS_NAMESPACE);
        return 0;
    }
    ESP_LOGI(TAG, "start init keys");
    nvs_purge_all(hNVS);
    ret = nvs_commit(hNVS);
    nvsCreateKeyStr(CFG_NVS_KEY_WIFI_SSID, CONFIG_MY_WIFI_SSID);
    nvsCreateKeyStr(CFG_NVS_KEY_WIFI_PASSWORD, CONFIG_MY_WIFI_PASSWORD);
    nvsCreateKeyStr(CFG_NVS_KEY_BT_DEVICE_NAME, CONFIG_MY_BT_DEVICE_NAME);
    nvsCreateKeyStr(CFG_NVS_KEY_MQTT_URL, CONFIG_BROKER_URL);
    nvsCreateKeyStr(CFG_NVS_KEY_MQTT_USERNAME); // CONFIG_MQTT_CLIENT_USERNAME
    nvsCreateKeyStr(CFG_NVS_KEY_MQTT_PASSWORD); //CONFIG_MQTT_CLIENT_PASSWORD
    nvsCreateKeyStr(CFG_NVS_KEY_NTP_SERVER,CONFIG_NTP_SERVER);
    nvsCreateKeyStr(CFG_NVS_KEY_LOCALE_TZ,CONFIG_DEFAULT_LOCALE_TIME_ZONE);
    ret = nvs_commit(hNVS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed (%d) nvs_commit", ret);
    }
    nvs_close(hNVS);
    return ESP_OK;
}
