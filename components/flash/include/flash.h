#ifndef FLASH_H
#define FLASH_H

#include <array>

#include "esp_err.h"
#if !defined(nvs_handle_t)
typedef uint32_t nvs_handle_t;
#endif

static constexpr const char* CFG_NVS_KEY_WIFI_SSID = "wifi_ssid";
static constexpr const char* CFG_NVS_KEY_WIFI_PASSWORD = "wifi_password";
static constexpr const char* CFG_NVS_KEY_DEVICE_UNIQUE = "device_unique";
static constexpr const char* CFG_NVS_KEY_MQTT_URL = "mqtt_url";
static constexpr const char* CFG_NVS_KEY_MQTT_USERNAME = "mqtt_uname";
static constexpr const char* CFG_NVS_KEY_MQTT_PASSWORD = "mqtt_pword";
static constexpr const char* CFG_NVS_KEY_NTP_SERVER = "ntp_server";
static constexpr const char* CFG_NVS_KEY_LOCALE_TZ = "locale_tz";

esp_err_t init_flash(void);
/* use C++ for handler container */
class NvsConfig {
    nvs_handle_t handler_ = 0;
public:
    explicit operator bool() const {
        return handler_ != 0;
    }
    explicit operator nvs_handle_t() const {
        return handler_;
    }
    NvsConfig();
    ~NvsConfig();
    NvsConfig(const NvsConfig&) = delete;
    NvsConfig& operator=(const NvsConfig&) = delete;
    NvsConfig(NvsConfig&&) = delete;
    NvsConfig& operator=(NvsConfig&&) = delete;
    bool setStr(const char* key, const char* value);
    size_t getStrLen(const char* key);
    bool getStr(const char *key, char *value, size_t len);
    bool getStr(const char *key, uint8_t *value, size_t len) {
        return getStr(key, (char *)value, len);
    }
    bool isStrEmpty(const char *key) const;
};

#endif