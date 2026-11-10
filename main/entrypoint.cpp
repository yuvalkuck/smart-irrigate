#include <array>
#include <cstdio>

#include "protocol.h"
#include "telemetry.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "logger.h"
#include "esp_event.h"
#include "flash.h"
#include "hwifi.h"
#include "wifi_softap.h"
#include "hmqtt.h"
#include "hsntp.h"
#include "hmodbus.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "time.h"
#include "gpio_declaraion.h"
#include "onewire_bus.h"
#include "esp_idf_version.h"
#include "esp_board_manager_includes.h"

enum blinkPattern_t {
    AllGood = 0,
    SetupMode = 1,
    Error = 2,
};
const uint16_t blinkPattern[] = {1000,300,100};
static constexpr auto TAG = "App:";

void init_event_app_handle();

static void setLedState(int fliper) {
    // ESP_LOGI(TAG, "set %d", fliper);
    gpio_set_level((gpio_num_t)LED_Pin, fliper);
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
    } else {
        ESP_LOGE(TAG, "failed to set timezone from fkasg");
    }
}

// Flexible Valve Pin Mapping Structure
// Instantiates size allocation from Kconfig dynamic targets
std::array<gpio_num_t, CONFIG_DEVICE_MAX_VALVES> valve_gpio_pins{};

// ====================================================================
// INITIALIZATION METHOD
// ====================================================================
extern onewire_bus_handle_t onewire_bus_handle;
static void init_gpio() {
    // LED and config-mode switch are set up by the board manager (components/board_manager).

    // === 2. Relay Array Controls ===
    // "gpio_valve_<n>" peripherals are configured and driven low by the board manager.
    // DEVICE_MAX_VALVES must not exceed the number of gpio_valve_<n> entries in board_peripherals.yaml.
    for (size_t i = 0; i < valve_gpio_pins.size(); ++i) {
        char name[16];
        snprintf(name, sizeof(name), "gpio_valve_%u", static_cast<unsigned>(i));
        void *periph = nullptr;
        ESP_ERROR_CHECK(esp_board_manager_get_periph_handle(name, &periph));
        valve_gpio_pins[i] = static_cast<periph_gpio_handle_t *>(periph)->gpio_num;
    }

    // === 4. 1-Wire Serial Interface (DS18B20 Soil Thermal) ===
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = ONEWIRE_Pin,
        .flags = {
            .en_pull_up = true,   // matches your original GPIO_PULLUP_ENABLE fallback
        },
    };
    onewire_bus_rmt_config_t rmt_config = {
        .max_rx_bytes = 10,
    };

    ESP_ERROR_CHECK(onewire_new_bus_rmt(&bus_config, &rmt_config, &onewire_bus_handle));
}

extern adc_oneshot_unit_handle_t adc_handle;

static void init_adc() {
    // The board manager creates ADC1 and configures Channel 2 (GPIO 2, Water Pressure) via "adc_pressure".
    // It supports one channel per oneshot unit, so Channel 3 (GPIO 3, Wind Speed) is added on the same handle.
    void *periph = nullptr;
    ESP_ERROR_CHECK(esp_board_manager_get_periph_handle("adc_pressure", &periph));
    adc_handle = static_cast<periph_adc_handle_t *>(periph)->oneshot;

    const adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, WIND_EXPANSION_Channel, &config));
}

extern "C" [[noreturn]] void app_main(void) {
    METHODTRACE
    esp_board_manager_init();
    init_gpio();
    init_adc();
    //Initialize NVS
    bool initRegular = (gpio_get_level(CONFIG_MODE_Pin) == 0 ? false : true);
    esp_err_t ret = init_flash();

    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "start event loop default");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    setLedState(true);
    blinkPattern_t initModePattern = AllGood;
    // test if we have wifi config
    { // block to release hCfg when not use anymore

        NvsConfig hCfg;
        if (!hCfg) {
            ESP_LOGE(TAG, "failed to init config flash");
            abort();
        }
        if (!initRegular || hCfg.isStrEmpty(CFG_NVS_KEY_WIFI_SSID)) {
            ESP_LOGI(TAG, "Start wifi configuration state");
            init_wifi_softap();
            start_wifi();
            initModePattern = SetupMode;
            ESP_LOGI(TAG, "free heap: %iK", esp_get_free_heap_size()/1024);
        } else {
            ESP_LOGI(TAG, "Start Regular Load");
            if ( ESP_OK == init_telemetry()) {
                init_event_app_handle();
                init_wifi_sta();
                // // 6. Start Wi-Fi
                start_wifi();
                // SNTP
                char buff[128] = {0};
                if (hCfg.getStr(CFG_NVS_KEY_NTP_SERVER, buff, sizeof(buff))) {
                    init_sntp(buff, continue_after_time_sync_cb);
                } else {
                    ESP_LOGE(TAG, "failed to get NTP server url from flash");
                }
                // MQTT
                init_hmqtt();
                if (init_modbus() != ESP_OK) {
                    ESP_LOGE(TAG, "RS-485 init failed");
                }
                start_sntp();
                start_telemetry();
            } else {
                initModePattern = Error;
            }
        }
    }
    ESP_LOGI(TAG, "working stage");
    auto ticker = blinkPattern[initModePattern];
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
