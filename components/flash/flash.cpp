#include "nvs_flash.h"
#include "esp_log.h"
#include "flash.h"
#include "sdkconfig.h"

#define NVS_NAMESPACE "."
#define NVS_PARTITION_NAME "config"

static const char* TAG = "flash:";
static nvs_handle_t hNVS;
esp_err_t nvs_set_str(uint32_t handle, const char* key, int value, size_t* len);

static void nvsCreateKeyStr(const char *name, const char *defaule_value = "") {
    ESP_LOGI(TAG, "%s:%s", __func__,name);
    esp_err_t err;
    size_t len = 0;
    char tmp[64]={0};
    // Step 1: Get the size of the stored string (includes null terminator)
    err = nvs_get_str(hNVS, name, tmp, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        err = nvs_set_str(hNVS, name, defaule_value);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(TAG, "failed to create: %s",defaule_value);
        }
    }
}
static void nvsCreateKeyInt(const char *name, long defaule_value = 0) {
    ESP_LOGI(TAG, "%s:%s", __func__,name);
    esp_err_t err;
    int32_t value;
    // Step 1: Get the size of the stored string (includes null terminator)
    err = nvs_get_i32(hNVS, name, &value);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        err = nvs_set_i32(hNVS, name, defaule_value);
        if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(TAG, "failed to create: %d", defaule_value);
        }
    }
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
    ret = nvs_open_from_partition(NVS_PARTITION_NAME,NVS_NAMESPACE, NVS_READWRITE, &hNVS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed (%d) to open partition: %s:%s",ret, NVS_PARTITION_NAME,NVS_NAMESPACE);
        return 0;
    }
    ESP_LOGI(TAG, "start init keys");

    nvsCreateKeyStr("wifi_ssid", CONFIG_MY_WIFI_SSID);
    nvsCreateKeyStr("wifi_password",CONFIG_MY_WIFI_PASSWORD);
    nvsCreateKeyStr("bt_password");
    nvsCreateKeyStr("bt_device_name", CONFIG_MY_BT_DEVICE_NAME);
    nvsCreateKeyStr("broker_url", CONFIG_BROKER_URL);
    nvsCreateKeyInt("broker_port", CONFIG_BROKER_PORT);
    nvsCreateKeyStr("mqtt_username", CONFIG_MQTT_CLIENT_USERNAME);
    nvsCreateKeyStr("mqtt_password", CONFIG_MQTT_CLIENT_PASSWORD);
    nvsCreateKeyStr("ntp_server",CONFIG_NTP_SERVER);
    nvsCreateKeyStr("locale_tz",CONFIG_DEFAULT_LOCALE_TIME_ZONE);
    ret = nvs_commit(hNVS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed (%d) nvs_commit",ret);
    }
    nvs_close(hNVS);
    return ESP_OK;
}