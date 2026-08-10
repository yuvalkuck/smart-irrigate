#include "esp_log.h"
#include "flash.h"

#include <algorithm>

#include "sdkconfig.h"
#include "nvs_flash.h"
#include "default_initiate.h"

static constexpr const char* PARTITION_SETUP_NAMESPACE_STR = ".";
static constexpr const char* PARTITION_SETUP_NAME_STR = "setup";

static const char* TAG = "flash:";

esp_err_t nvsCreateKeyStr(nvs_handle_t handler, const char* name, const char* default_value = "") {
    ESP_LOGD(TAG, "%s: %s", __func__, name);
    size_t required_size;
    esp_err_t err = nvs_get_str(handler, name, NULL, &required_size);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        err = nvs_set_str(handler, name, default_value);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to set default value for key '%s': %s", name, esp_err_to_name(err));
        }
    }
    return err;
}

static esp_err_t nvs_config(nvs_handle_t* handler) {
    esp_err_t ret = nvs_open_from_partition(PARTITION_SETUP_NAME_STR, PARTITION_SETUP_NAMESPACE_STR, NVS_READWRITE, handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed (%d) to open partition: %s:%s", ret, PARTITION_SETUP_NAME_STR, PARTITION_SETUP_NAMESPACE_STR);
    }
    return ret;
}

NvsConfig::NvsConfig() {
    nvs_config(&handler_);
}

NvsConfig::~NvsConfig() {
    nvs_close(handler_);
}

bool NvsConfig::isStrEmpty(const char* key) const {
    size_t required_size;
    esp_err_t err = nvs_get_str(handler_, key, NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return true;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error checking if key '%s' is empty: %s", key, esp_err_to_name(err));
        return true;
    }
    return required_size <= 1; // Only null terminator or truly empty
}
size_t NvsConfig::getStrLen(const char* key) {
    size_t required_size;
    auto err = nvs_get_str(handler_, key, NULL, &required_size);
    if (err != ESP_OK) {
        return 0;
    }
    return required_size;

}
bool NvsConfig::getStr(const char* key, char* value, size_t len) {
    size_t required_size;
    esp_err_t err;

    // 1. Get the required size of the string (including null terminator)
    err = nvs_get_str(handler_, key, NULL, &required_size);
    if (err != ESP_OK) {
        if (err != ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGE(TAG, "Error (%s) getting size for NVS key '%s'", esp_err_to_name(err), key);
        }
        return false; // Key not found or other error
    }

    // 2. Check if the provided buffer is large enough
    if (len < required_size) {
        ESP_LOGE(TAG, "Buffer too small for NVS key '%s'. Required: %zu, Provided: %zu", key, required_size, len);
        return false;
    }

    // 3. Read the string into the provided buffer
    err = nvs_get_str(handler_, key, value, &required_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) reading NVS key '%s'", esp_err_to_name(err), key);
        return false;
    }

    return true; // Success
}

bool NvsConfig::setStr(const char* key, const char* value) {
    esp_err_t err = nvs_set_str(handler_, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) writing NVS key '%s'", esp_err_to_name(err), key);
        return false;
    }
    err = nvs_commit(handler_);
    return err == ESP_OK;
}

esp_err_t init_flash(void) {
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
            ESP_LOGE(TAG, "Failed to re-initialize NVS partition '%s' after erase: %s", PARTITION_SETUP_NAME_STR, esp_err_to_name(ret));
            return ret;
        }
    }

    nvs_handle_t setup_handle;
    ret = nvs_config(&setup_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed (%d) to open partition: %s:%s", ret, PARTITION_SETUP_NAME_STR, PARTITION_SETUP_NAMESPACE_STR);
        return ret;
    }

    ESP_LOGI(TAG, "start init keys");
    nvsCreateKeyStr(setup_handle, CFG_NVS_KEY_WIFI_SSID, INITIATE_WIFI_SSID);
    nvsCreateKeyStr(setup_handle, CFG_NVS_KEY_WIFI_PASSWORD, INITIATE_WIFI_PASSWORD);
    nvsCreateKeyStr(setup_handle, CFG_NVS_KEY_DEVICE_UNIQUE, INITIATE_DEVICE_UNIQUE);
    nvsCreateKeyStr(setup_handle, CFG_NVS_KEY_MQTT_URL, INITIATE_BROKER_URL);
    nvsCreateKeyStr(setup_handle, CFG_NVS_KEY_MQTT_USERNAME, INITIATE_MQTT_CLIENT_USERNAME);
    nvsCreateKeyStr(setup_handle, CFG_NVS_KEY_MQTT_PASSWORD, INITIATE_MQTT_CLIENT_PASSWORD);
    nvsCreateKeyStr(setup_handle, CFG_NVS_KEY_NTP_SERVER, INITIATE_NTP_SERVER);
    nvsCreateKeyStr(setup_handle, CFG_NVS_KEY_LOCALE_TZ, INITIATE_DEFAULT_LOCALE_TIME_ZONE);

    ret = nvs_commit(setup_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed (%d) nvs_commit", ret);
    }
    nvs_close(setup_handle);
    return ESP_OK;
}