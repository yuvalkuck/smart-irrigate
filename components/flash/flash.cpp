#include "esp_log.h"
#include "flash.h"
#include "sdkconfig.h"
#include "nvs_flash.h"

static constexpr const char* PARTITION_SETUP_NAMESPACE_STR = ".";
static constexpr const char* PARTITION_SETUP_NAME_STR = "setup";

static const char* TAG = "flash:";
extern esp_err_t nvs_config(nvs_handle_t* handler);

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

size_t NvsConfig::getStrLen(const char* key) const {
    size_t required_size;
    auto err = nvs_get_str(handler_, key, NULL, &required_size);
    if (err != ESP_OK) {
        return 0;
    }
    return required_size;
}

bool NvsConfig::getStr(const char* key, char* value, size_t len) const {
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

bool NvsConfig::setStr(const char* key, const char* value) const {
    esp_err_t err = nvs_set_str(handler_, key, value); // it does not change handler so it can be const
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) writing NVS key '%s'", esp_err_to_name(err), key);
        return false;
    }
    err = nvs_commit(handler_);
    return err == ESP_OK;
}

bool NvsConfig::getMask(const char* key, uint16_t& value) const {
    size_t required_size;
    esp_err_t err = nvs_get_blob(handler_, key, &value, &required_size);
    // it does not change handler so it can be const
    if (err != ESP_OK || required_size != sizeof(uint16_t)) {
        ESP_LOGE(TAG, "Error (%s) get NVS mask key '%s'", esp_err_to_name(err), key);
        return false;
    }
    return err == ESP_OK;
}

bool NvsConfig::maskAddSub_(bool isAdd, const char* key, uint16_t value) {
    uint16_t nvs_value, compare_with;
    if (!getMask(key, nvs_value)) {
        return false;
    }
    compare_with = nvs_value;
    if (isAdd) {
        nvs_value |= value;
    } else {
        nvs_value &= value;
    }
    if (nvs_value == compare_with) {
        return true;
    }
    esp_err_t err = nvs_set_blob(handler_, key, &nvs_value, sizeof(nvs_value));
    // it does not change handler so it can be const
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) writing NVS mask key '%s'", esp_err_to_name(err), key);
        return false;
    }
    err = nvs_commit(handler_);
    return err == ESP_OK;
}
