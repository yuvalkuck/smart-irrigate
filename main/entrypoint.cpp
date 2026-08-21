#include <array>
#include <string_view>
#include <charconv>

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
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "time.h"
#include "gpio_declaraion.h"
#include "onewire_bus.h"
#if defined(ESP32S3_UART)
#include "driver/uart.h"
#endif

static constexpr auto TAG = "App:";

void init_event_app_handle();

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
    // === Status Indication ===
    gpio_reset_pin(GPIO_LED);
    gpio_set_direction(GPIO_LED, GPIO_MODE_OUTPUT);

    // === 1. Local Configuration Mode Intercept Switch ===
    gpio_reset_pin(GPIO_CONFIG_MODE_PIN);
    const gpio_config_t config_mode_switch_conf = {
        .pin_bit_mask = (1ULL << GPIO_CONFIG_MODE_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE, // CHANGED: Disabled internal pull-up to use external 10k resistor
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&config_mode_switch_conf);

    // === 2. Parse Kconfig String via Zero-Allocation C++17 string_view ===
    std::string_view pin_list_view{CONFIG_DYNAMIC_VALVE_GPIO_LIST};
    size_t parsed_valve_count = 0;

    while (!pin_list_view.empty() && parsed_valve_count < CONFIG_DEVICE_MAX_VALVES) {
        const size_t comma_pos = pin_list_view.find(',');
        const std::string_view token = (comma_pos == std::string_view::npos)
                                           ? pin_list_view
                                           : pin_list_view.substr(0, comma_pos);

        if (!token.empty()) {
            int pin_num = 0;
            auto [ptr, ec] = std::from_chars(token.data(), token.data() + token.size(), pin_num);
            if (ec == std::errc{}) {
                static constexpr gpio_num_t reserved_pins[] = {
                    GPIO_LED, GPIO_CONFIG_MODE_PIN, GPIO_ONEWIRE_BUS, GPIO_WIND_EXPANSION,
                    I2C_SDA_PIN, I2C_SCL_PIN
                };
                bool reserved = false;
                for (auto reserved_pin : reserved_pins) {
                    if (pin_num == reserved_pin) {
                        reserved = true;
                        break;
                    }
                }
                if (reserved) {
                    ESP_LOGE(TAG, "DYNAMIC_VALVE_GPIO_LIST: GPIO%d is reserved for another peripheral, skipping",
                             pin_num);
                } else {
                    valve_gpio_pins[parsed_valve_count] = static_cast<gpio_num_t>(pin_num);
                    parsed_valve_count++;
                }
            }
        }

        if (comma_pos == std::string_view::npos) {
            break;
        }
        pin_list_view.remove_prefix(comma_pos + 1);
    }

    // === 3. Relay Array Controls ===
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

    for (size_t i = 0; i < parsed_valve_count; ++i) {
        gpio_set_level(valve_gpio_pins[i], 0);
    }

    // === 4. 1-Wire Serial Interface (DS18B20 Soil Thermal) ===
    onewire_bus_config_t bus_config = {
        .bus_gpio_num = GPIO_ONEWIRE_BUS,
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
    // 1. Configure the ADC1 Unit Configuration Structure
    const adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_DIGI_CLK_SRC_DEFAULT,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc_handle));

    // 2. Define Shared Channel Properties
    const adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    // 3. Register Channel 2 (GPIO 2, Water Pressure) and Channel 3 (GPIO 3, Wind Speed) sequentially
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_2, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_3, &config));
}
#if defined(ESP32S3_UART)
static void init_s3_uart_link() {
    const uart_config_t uart_config = {
        .baud_rate = 115200,          // Standard stable cross-chip transfer rate
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Install the driver using the defined port configuration
    ESP_ERROR_CHECK(uart_driver_install(S3_LINK_UART_PORT, S3_LINK_UART_BUFF * 2, 0, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(S3_LINK_UART_PORT, &uart_config));

    // Route the hardware matrix signals directly to your specific safe macro pins
    ESP_ERROR_CHECK(uart_set_pin(S3_LINK_UART_PORT, GPIO_S3_LINK_TX, GPIO_S3_LINK_RX, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}
#endif

extern "C" [[noreturn]] void app_main(void) {
    METHODTRACE
    init_gpio();
    init_adc();
#if defined(ESP32S3_UART)
    init_s3_uart_link();
#endif
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
            abort();
        }
        if (!initRegular || hCfg.isStrEmpty(CFG_NVS_KEY_WIFI_SSID)) {
            ESP_LOGI(TAG, "Start wifi configuration state");
            init_wifi_softap();
            start_wifi();
            initRegular = false;
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
                    if ( init_sntp(buff, continue_after_time_sync_cb) != ESP_OK) {
                        ESP_LOGE(TAG, "failed to init sntp");
                    } else {
                        if ( start_sntp() != ESP_OK) {
                            ESP_LOGE(TAG, "failed to start sntp");
                        }
                    }
                } else {
                    ESP_LOGE(TAG, "failed to get NTP server url from flash");
                }
                // MQTT
                init_hmqtt();

                start_telemetry();
            } else {
                initRegular = false;
            }
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
