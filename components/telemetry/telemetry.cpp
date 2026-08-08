#include "telemetry.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "logger.h"
#include "sht4x.h"
#include "common_event.h"
#include "esp_event.h"
#include "diagnostics.h"
#include "esp_adc/adc_oneshot.h"

// Define I2C pins for ESP32-C6
#define I2C_PORT       I2C_NUM_0
#define I2C_SDA_PIN       GPIO_NUM_19
#define I2C_SCL_PIN       GPIO_NUM_20
#define TASK_DELAY     1000*10


static const char* TAG = "Telemetry:";
static TelemetryData telemetryValues;
static adc_oneshot_unit_handle_t adc_handle;
static adc_oneshot_unit_init_cfg_t adcConfig = {
   .unit_id = ADC_UNIT_1,
   .clk_src = static_cast<adc_oneshot_clk_src_t>(0),                    // 0 auto-selects default architecture clock
   .ulp_mode = ADC_ULP_MODE_DISABLE, // C++ error if out of order!
};

static i2c_master_bus_handle_t bus_handle;
static i2c_master_bus_config_t bus_config = {
    .i2c_port = I2C_PORT,
    .sda_io_num = I2C_SDA_PIN,
    .scl_io_num = I2C_SCL_PIN,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7
};

static sht4x_handle_t sht41_handle{};
static sht4x_config_t sht41_config = {
    .i2c_address    = I2C_SHT4X_DEV_ADDR_LO,
    .i2c_clock_speed= I2C_SHT4X_DEV_CLK_SPD,
    .repeat_mode    = SHT4X_REPEAT_HIGH,
    .heater_mode    = SHT4X_HEATER_OFF
};

esp_err_t init_telemetry() {
    METHODTRACE
    /***** init services ****/
    bus_config.flags.enable_internal_pullup = true;
    // I2C Master
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    // ADC-1
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adcConfig, &adc_handle));
    adc_oneshot_chan_cfg_t chan_config = {
        .atten = ADC_ATTEN_DB_12,         // Safe voltage range (~0V to 3.3V)
        .bitwidth = ADC_BITWIDTH_DEFAULT, // Hardware native resolution (12-bit for C6)
    };
    // For ESP32-C6 ADC1, ADC_CHANNEL_0 maps to GPIO0
    // auto rc = adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_0, &chan_config);
    // if (rc != ESP_OK) {
    //     ESP_ERROR_CHECK(rc);
    //     return rc;
    // }
    /***** Register sensors ****/
    auto rc = sht4x_init(bus_handle, &sht41_config, &sht41_handle);
    if (rc != ESP_OK) {
        ESP_ERROR_CHECK(rc);
        return rc;
    }

    auto frc = execute_operational_self_test(bus_handle, adc_handle);
    if ( frc != 0) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

[[noreturn]] static void cbTelemetryTask(void* ) {
    while (1) {
        // Read sensor data using the high-precision mode
        auto res = sht4x_get_measurement(sht41_handle, &telemetryValues.temperature, &telemetryValues.humidity);
        if (res == ESP_OK) {
            ESP_LOGI(TAG, "Temperature: %.2f °C | Humidity: %.2f %%", telemetryValues.temperature, telemetryValues.humidity);
            float payload[2] = {
                telemetryValues.temperature,    //!< temperature in degree C        (Invalid value -327.68)
                telemetryValues.humidity       //!< relative humidity in %         (Invalid value 0.0)
            };
            EventData event = {sizeof(payload), &payload};
            esp_event_post(COMMON_BASE_EVENTS, COMMON_EVENT_SENSOR_UPDATED, &event, event.length(), portMAX_DELAY);
        } else {
            ESP_LOGE(TAG, "Failed to read data from sensor");
        }
        vTaskDelay(pdMS_TO_TICKS(TASK_DELAY));
    }
}

void start_telemetry() {
    METHODTRACE
    BaseType_t result = xTaskCreate(
        cbTelemetryTask,
        "TelemetryTask",
        4096,
        NULL,
        5,
        NULL
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task due to insufficient memory!");
    }
}
