#include "esp_log.h"
#include "flash.h"
#include "sdkconfig.h"
#include "nvs_flash.h"

#define NVS_NAMESPACE "."
#define NVS_PARTITION_NAME "config"

static const char* TAG = "flash:";
static nvs_handle_t hNVS;
// esp_err_t nvs_set_str(uint32_t handle, const char* key, int value, size_t* len);
bool nvs_key_isempty(const uint32_t *handler, const char *name) {
    char tmp[64] = {0};
    size_t len = sizeof(tmp);
    auto err = nvs_get_str(hNVS, name, tmp, &len);
    ESP_LOGI(TAG, "%s:%s:%d", __func__, tmp,len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return true;
    }
    if ( strlen(tmp) < 1) {
        return true;
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

esp_err_t nvs_config(nvs_handle_t* handler) {
    esp_err_t ret = nvs_open_from_partition(NVS_PARTITION_NAME,NVS_NAMESPACE, NVS_READWRITE, handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed (%d) to open partition: %s:%s", ret, NVS_PARTITION_NAME, NVS_NAMESPACE);
    }
    return ret;
}

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
