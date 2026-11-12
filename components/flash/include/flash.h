#ifndef FLASH_H
#define FLASH_H

#include <array>

#include "esp_err.h"
#if !defined(nvs_handle_t)
typedef uint32_t nvs_handle_t;
#endif
// TODO: need to convert SETUP_KEY to template for type verify
#define SETUP_KEY(name) SETUP_NVS_KEY_##name

#define DEC_SETUP_KEY(name, value) constexpr const char* SETUP_NVS_KEY_##name = #value
DEC_SETUP_KEY(WIFI_SSID, wifi_ssid);
DEC_SETUP_KEY(WIFI_PASSWORD,wifi_password);
DEC_SETUP_KEY(DEVICE_UNIQUE,device_unique);
DEC_SETUP_KEY(MQTT_URL,mqtt_url);
DEC_SETUP_KEY(MQTT_USERNAME,mqtt_uname);
DEC_SETUP_KEY(MQTT_PASSWORD,mqtt_pword);
DEC_SETUP_KEY(NTP_SERVER,ntp_server);
DEC_SETUP_KEY(LOCALE_TZ,locale_tz);
#undef DEC_SETUP_KEY
esp_err_t init_flash();

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
    bool setStr(const char* key, const char* value) const;
    size_t getStrLen(const char* key) const;
    bool getStr(const char* key, char* value, size_t len) const;

    bool getStr(const char* key, uint8_t* value, size_t len) const {
        return getStr(key, (char*)value, len);
    }

    bool isStrEmpty(const char* key) const;
};

#endif
