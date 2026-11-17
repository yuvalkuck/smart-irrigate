#ifndef FLASH_H
#define FLASH_H

#include <array>
#include <concepts>

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

enum class SoftwareEnable : uint16_t {
    Disable = 0x00,
    Mqtt    = 0x01,
    Enable  = 0xFFFF
};

enum class HardwareEnable : uint32_t {
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
    Battery            = 0x400,
    Enable             = 0xFFFFFFFF
};

// HW enable
#undef SETUP_KEY_NAME
esp_err_t init_flash();
// the following set, we do not use all - fo row it staties
inline uint32_t operator|(HardwareEnable lhs, HardwareEnable rhs) {
    return static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs);
}

inline uint32_t operator&(HardwareEnable lhs, HardwareEnable rhs) {
    return static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs);
}

inline uint16_t operator|(SoftwareEnable lhs, SoftwareEnable rhs) {
    return static_cast<uint16_t>(lhs) | static_cast<uint16_t>(rhs);
}

inline uint16_t operator&(SoftwareEnable lhs, SoftwareEnable rhs) {
    return static_cast<uint16_t>(lhs) & static_cast<uint16_t>(rhs);
}


/* use C++ for handler container */
class NvsConfig {
    nvs_handle_t handler_ = 0;
    template<typename TY>
    requires std::unsigned_integral<TY>
    bool maskAddSub_(bool isAdd, const char* key, TY value) const;

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
    template<typename TY>
    requires std::unsigned_integral<TY>
    bool getMask(const char* key, TY& value) const;

    template<typename TY>
    requires std::unsigned_integral<TY>
    bool addMask(const char* key, TY value) {
        return maskAddSub_(true, key, value);
    }
    template<typename TY>
    requires std::unsigned_integral<TY>
    bool subMask(const char* key, TY value) {
        return maskAddSub_(false, key, value);
    }
};

#endif
