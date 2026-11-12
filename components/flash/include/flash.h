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
// boolean SW/HW
SETUP_KEY_NAME(ENABLE_SW, enable_softw);
SETUP_KEY_NAME(ENABLE_HW, enable_hardw);

enum class SoftwareEnable : uint8_t {
    Disable = 0x00,
    Mqtt    = 0x01,
    Enable  = 0xFF
};

enum class HardwareEnable : uint16_t {
    Disable            = 0x00,
    Display            = 0x01,
    DeviceAI           = 0x02,
    PlatformSensor     = 0x04,
    SensorBME          = 0x08,
    SensorSHT          = 0x10,
    SensorSolar        = 0x20,
    SensorSoile        = 0x40,
    SensorPresher      = 0x80,
    SensorWindBoard    = 0x100,
    SensorWindPlatform = 0x200,
    Enable             = 0xFFFF
};

// HW enable
#undef SETUP_KEY_NAME
esp_err_t init_flash();

inline uint16_t operator|(HardwareEnable lhs, HardwareEnable rhs) {
    return lhs | rhs;
}

inline uint16_t operator&(HardwareEnable lhs, HardwareEnable rhs) {
    return lhs & rhs;
}

/* use C++ for handler container */
class NvsConfig {
    nvs_handle_t handler_ = 0;
    bool maskAddSub_(bool isAdd, const char* key, uint16_t value) const;

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
    // string
    bool setStr(const char* key, const char* value) const;
    size_t getStrLen(const char* key) const;
    bool getStr(const char* key, char* value, size_t len) const;

    bool getStr(const char* key, uint8_t* value, size_t len) const {
        return getStr(key, (char*)value, len);
    }

    bool isStrEmpty(const char* key) const;
    // mask
    bool getMask(const char* key, uint16_t& value) const;

    bool addMask(const char* key, uint16_t value) {
        return maskAddSub_(true, key, value);
    }

    bool subMask(const char* key, uint16_t value) {
        return maskAddSub_(false, key, value);
    }
};

#endif
