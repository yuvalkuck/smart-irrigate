#ifndef FLASH_H
#define FLASH_H

#include <array>

#include "esp_err.h"
#if !defined(nvs_handle_t)
typedef uint32_t nvs_handle_t;
#endif
// TODO: need to convert SETUP_KEY to template for type verify
#define SETUP_KEY(name) SETUP_NVS_KEY_##name

#define SETUP_KEY_NAME(name, value) constexpr const char* SETUP_NVS_KEY_##name = #value
SETUP_KEY_NAME(WIFI_SSID, wifi_ssid);
SETUP_KEY_NAME(WIFI_PASSWORD, wifi_password);
SETUP_KEY_NAME(DEVICE_UNIQUE, device_unique);
SETUP_KEY_NAME(MQTT_URL, mqtt_url);
SETUP_KEY_NAME(MQTT_USERNAME, mqtt_uname);
SETUP_KEY_NAME(MQTT_PASSWORD, mqtt_pword);
SETUP_KEY_NAME(NTP_SERVER, ntp_server);
SETUP_KEY_NAME(LOCALE_TZ, locale_tz);
//
SETUP_KEY_NAME(ENABLE_LCD, enable_lcd);
SETUP_KEY_NAME(ENABLE_AI, enable_aidev);
SETUP_KEY_NAME(PLATFORM_SENSOR, plat_sensors);
SETUP_KEY_NAME(SENSOR_BME, plat_sensors);
SETUP_KEY_NAME(SENSOR_SHT, sensor_sht);
SETUP_KEY_NAME(SENSOR_SOLAR, sensor_solar);
SETUP_KEY_NAME(SENSOR_WIND, sensor_wind);
SETUP_KEY_NAME(SENSOR_SOILE, sensor_soile);
SETUP_KEY_NAME(SENSOR_PRESHER, sensor_presher);
#undef SETUP_KEY_NAME
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
