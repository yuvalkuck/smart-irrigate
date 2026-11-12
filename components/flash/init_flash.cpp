#include "esp_log.h"
#include "flash.h"

#include <algorithm>

#include "sdkconfig.h"
#include "nvs_flash.h"
#include "default_initiate.h"
static const char* TAG = "initFlash:";
static constexpr const char* PARTITION_SETUP_NAMESPACE_STR = ".";
static constexpr const char* PARTITION_SETUP_NAME_STR = "setup";
extern esp_err_t nvsCreateKeyStr(nvs_handle_t handler, const char* name, const char* default_value = "");


esp_err_t nvs_config(nvs_handle_t* handler) {
    esp_err_t ret = nvs_open_from_partition(PARTITION_SETUP_NAME_STR, PARTITION_SETUP_NAMESPACE_STR, NVS_READWRITE,
                                            handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed (%d) to open partition: %s:%s", ret, PARTITION_SETUP_NAME_STR,
                 PARTITION_SETUP_NAMESPACE_STR);
    }
    return ret;
}
#define SET_SETUP_KEY(name, value) nvsCreateKeyStr(setup_handle, SETUP_NVS_KEY_##name, value);
#define SET_SETUP_CFGKEY(name, value) nvsCreateKeyStr(setup_handle, SETUP_NVS_KEY_##name, #value);
#define ENABLE "1"
#define DISABLE "0"
esp_err_t init_flash() {
    ESP_LOGI(TAG, "nvs flash init");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ret = nvs_flash_erase();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to erase NVS flash: %s", esp_err_to_name(ret));
            return ret;
        }
        ret = nvs_flash_init();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to re-initialize NVS flash after erase: %s", esp_err_to_name(ret));
            return ret;
        }
    }
    ret = nvs_flash_init_partition(PARTITION_SETUP_NAME_STR);
    if (ret != ESP_OK) {
        ret = nvs_flash_erase_partition(PARTITION_SETUP_NAME_STR);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to erase NVS partition '%s': %s", PARTITION_SETUP_NAME_STR, esp_err_to_name(ret));
            return ret;
        }
        ret = nvs_flash_init_partition(PARTITION_SETUP_NAME_STR);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to re-initialize NVS partition '%s' after erase: %s", PARTITION_SETUP_NAME_STR,
                     esp_err_to_name(ret));
            return ret;
        }
    }

    nvs_handle_t setup_handle;
    ret = nvs_config(&setup_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed (%d) to open partition: %s:%s", ret, PARTITION_SETUP_NAME_STR,
                 PARTITION_SETUP_NAMESPACE_STR);
        return ret;
    }

    ESP_LOGI(TAG, "start init keys");
    SET_SETUP_KEY(WIFI_SSID, INITIATE_WIFI_SSID);
    SET_SETUP_KEY(WIFI_PASSWORD, INITIATE_WIFI_PASSWORD);
    SET_SETUP_KEY(DEVICE_UNIQUE, INITIATE_DEVICE_UNIQUE);
    SET_SETUP_KEY(MQTT_URL, INITIATE_BROKER_URL);
    SET_SETUP_KEY(MQTT_USERNAME, INITIATE_MQTT_CLIENT_USERNAME);
    SET_SETUP_KEY(MQTT_PASSWORD, INITIATE_MQTT_CLIENT_PASSWORD);
    SET_SETUP_KEY(NTP_SERVER, INITIATE_NTP_SERVER);
    SET_SETUP_KEY(LOCALE_TZ, INITIATE_DEFAULT_LOCALE_TIME_ZONE);
    //
    SET_SETUP_CFGKEY(ENABLE_AI, CONFIG_DEVICE_PLATFORM_AI_ENABLE);
    SET_SETUP_CFGKEY(ENABLE_LCD, CONFIG_DEVICE_PLATFORM_AI_ENABLE); // lcd is in AI device
    //
    SET_SETUP_KEY(PLATFORM_SENSOR, ENABLE);
    SET_SETUP_KEY(SENSOR_BME, ENABLE);
    SET_SETUP_KEY(SENSOR_SHT, ENABLE);
    SET_SETUP_KEY(SENSOR_SOLAR, ENABLE);
    //
    SET_SETUP_KEY(SENSOR_WIND, ENABLE);
    SET_SETUP_KEY(SENSOR_SOILE, ENABLE);
    SET_SETUP_KEY(SENSOR_PRESHER, ENABLE);


    ret = nvs_commit(setup_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed (%d) nvs_commit", ret);
    }
    nvs_close(setup_handle);
    return ESP_OK;
}
