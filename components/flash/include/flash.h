#include "esp_err.h"

#define CFG_NVS_KEY_WIFI_SSID       "wifi_ssid"
#define CFG_NVS_KEY_WIFI_PASSWORD   "wifi_password"
#define CFG_NVS_KEY_BT_DEVICE_NAME  "device_name"
#define CFG_NVS_KEY_MQTT_URL        "mqtt_url"
#define CFG_NVS_KEY_MQTT_USERNAME   "mqtt_uname"
#define CFG_NVS_KEY_MQTT_PASSWORD   "mqtt_pword"
#define CFG_NVS_KEY_NTP_SERVER      "ntp_server"
#define CFG_NVS_KEY_LOCALE_TZ       "locale_tz"


esp_err_t init_flash(void);
esp_err_t nvs_config(uint32_t*);
bool nvs_key_isempty(const uint32_t handler, const char *name);
