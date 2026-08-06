#include <algorithm>
#include <array>
#include <string_view>
#include <charconv>

#include "protocol.h"
#include "telemetry.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_event.h"
#include "flash.h"
#include "hwifi.h"
#include "wifi_softap.h"
#include "hmqtt.h"
#include "hsntp.h"
#include "driver/gpio.h"
#include "time.h"

static const char* TAG = "App:";

void init_event_app_handle();

// ====================================================================
// CONFIGURABLE SYSTEM PINS (Declared right before the method)
// ====================================================================
#define GPIO_LED             GPIO_NUM_15  // Status indication LED
#define GPIO_CONFIG_MODE_PIN GPIO_NUM_17  // Manual Configuration Mode intercept switch
#define GPIO_ONEWIRE_BUS     GPIO_NUM_18  // 1-Wire data bus for DS18B20 soil thermometer
#define GPIO_WIND_EXPANSION  GPIO_NUM_2   // Reserved pulse input for future anemometer

static void setLedState(int fliper) {
    // ESP_LOGI(TAG, "set %d", fliper);
    gpio_set_level((gpio_num_t)GPIO_LED, fliper);
}

void continue_after_time_sync_cb(struct timeval* tv) {
    ESP_LOGI(TAG, "Notification of a time synchronization event");
    NvsConfig hCfg;
    char buff[128] = {0};
    if (hCfg.getStr(CFG_NVS_KEY_LOCALE_TZ, buff, sizeof(buff))) {
        setenv("TZ", buff, 1);
        tzset();
        ESP_LOGI(TAG, "set TimeZone to: %s", buff);
        start_hmqtt();
    }
    else {
        ESP_LOGE(TAG, "failed to set timezone from fkasg");
    }
}

// Flexible Valve Pin Mapping Structure
// Instantiates size allocation from Kconfig dynamic targets
static std::array<gpio_num_t, CONFIG_DEVICE_MAX_VALVES> valve_gpio_pins{};

// ====================================================================
// INITIALIZATION METHOD
// ====================================================================
static void init_gpio() {
    // === Status Indication ===
    gpio_reset_pin(GPIO_LED);
    gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);

    // === 1. Local Configuration Mode Intercept Switch ===
    gpio_reset_pin(GPIO_CONFIG_MODE_PIN);
    const gpio_config_t config_mode_switch_conf = {
        .pin_bit_mask = (1ULL << GPIO_CONFIG_MODE_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&config_mode_switch_conf);

    // === 2. Parse Kconfig String via Zero-Allocation C++17 string_view ===
    std::string_view pin_list_view{CONFIG_DYNAMIC_VALVE_GPIO_LIST};
    size_t parsed_valve_count = 0;

    while (!pin_list_view.empty() && parsed_valve_count < CONFIG_DEVICE_MAX_VALVES) {
        // Find position of next comma delimiter
        const size_t comma_pos = pin_list_view.find(',');
        const std::string_view token = (comma_pos == std::string_view::npos)
                                           ? pin_list_view
                                           : pin_list_view.substr(0, comma_pos);

        if (!token.empty()) {
            int pin_num = 0;
            // C++17 non-allocating, raw string parsing
            auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), pin_num);
            if (ec == std::errc{}) {
                valve_gpio_pins[parsed_valve_count] = static_cast<gpio_num_t>(pin_num);
                parsed_valve_count++;
            }
        }

        // Shift view past parsed segment
        if (comma_pos == std::string_view::npos) {
            break;
        }
        pin_list_view.remove_prefix(comma_pos + 1);
    }

    // === 3. Relay Array Controls (CONFIG_DEVICE_MAX_VALVES Programmatic Channels) ===
    uint64_t valve_pin_mask = 0;
    for (size_t i = 0; i < parsed_valve_count; ++i) {
        gpio_reset_pin(valve_gpio_pins[i]);
        valve_pin_mask |= (1ULL << valve_gpio_pins[i]);
    }

    const gpio_config_t valve_conf = {
        .pin_bit_mask = valve_pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&valve_conf);

    // Force active lines low immediately at boot to prevent floating valve triggers
    for (size_t i = 0; i < parsed_valve_count; ++i) {
        gpio_set_level(valve_gpio_pins[i], 0);
    }

    // === 4. 1-Wire Serial Interface (DS18B20 Soil Thermal) ===
    gpio_reset_pin(GPIO_ONEWIRE_BUS);
    const gpio_config_t onewire_conf = {
        .pin_bit_mask = (1ULL << GPIO_ONEWIRE_BUS),
        .mode = GPIO_MODE_INPUT_OUTPUT_OD,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&onewire_conf);

    // === 5. Future Wind Speed Expansion Pin (Anemometer) ===
    gpio_reset_pin(GPIO_WIND_EXPANSION);
    const gpio_config_t wind_conf = {
        .pin_bit_mask = (1ULL << GPIO_WIND_EXPANSION),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&wind_conf);
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "setting up");
    ESP_LOGI(TAG, "free heap: %iK", esp_get_free_heap_size()/1024);

    init_gpio();
    //Initialize NVS
    bool initRegular = (gpio_get_level(GPIO_CONFIG_MODE_PIN) == 0 ? false : true);
    esp_err_t ret = init_flash();

    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "start event loop default");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    setLedState(true);
    // test if we have wifi config
    { // block to release hCfg when not use anymore

        NvsConfig hCfg;
        if (!hCfg) {
            ESP_LOGE(TAG, "failed to init config flash");
            return;
        }
        if (!initRegular || hCfg.isStrEmpty(CFG_NVS_KEY_WIFI_SSID)) {
            ESP_LOGI(TAG, "Start wifi configuration state");
            init_wifi_softap();
            start_wifi();
            initRegular = false;
            ESP_LOGI(TAG, "free heap: %iK", esp_get_free_heap_size()/1024);
        }
        else {
            ESP_LOGI(TAG, "Start Regular Load");
            init_event_app_handle();
            init_telemetry();

            init_wifi_sta();
            // // 6. Start Wi-Fi
            start_wifi();
            // SNTP
            char buff[128] = {0};
            if (hCfg.getStr(CFG_NVS_KEY_NTP_SERVER, buff, sizeof(buff))) {
                init_sntp(buff, continue_after_time_sync_cb);
            }
            else {
                ESP_LOGE(TAG, "failed to get NTP server url from flash");
            }
            // MQTT
            init_hmqtt();
            start_sntp();
            start_telemetry();
        }
    }
    ESP_LOGI(TAG, "working stage");
    auto ticker = (initRegular ? 2000 : 250);
    int ledFlip = 1;
    for (;;) {
        --ledFlip;
        if (ledFlip < 0) {
            ledFlip = 1;
        }
        setLedState(ledFlip);
        vTaskDelay(ticker / portTICK_PERIOD_MS);
    }
}
