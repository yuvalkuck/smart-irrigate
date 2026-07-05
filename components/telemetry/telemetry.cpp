#include "telemetry.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "sht4x.h"
#include "common_event.h"
#include "esp_event.h"


// Define I2C pins for ESP32-C6
#define I2C_PORT       I2C_NUM_0
#define SDA_GPIO       GPIO_NUM_19
#define SCL_GPIO       GPIO_NUM_20
#define TASK_DELAY     1000*10
// BME680 default I2C Address (usually 0x76 or 0x77 depending on SDO pin state)
#define BME680_I2C_ADDR BME680_I2C_ADDR_1
static sht4x_t sensor;
static const char* TAG = "SHT41:";
static TelemetryData telemetryValues;

void init_telemetry() {
    ESP_LOGI(TAG, "%s", __func__);
    memset(&sensor, 0, sizeof(sht4x_t));
    // Initialize underlying thread-safe esp-idf-lib hardware driver wrapper
    ESP_ERROR_CHECK(i2cdev_init());

    ESP_ERROR_CHECK(sht4x_init_desc(&sensor, I2C_PORT, SDA_GPIO, SCL_GPIO));
    if ( sht4x_init(&sensor) != ESP_OK ) {
        ESP_LOGE(TAG, "Sensor initialization failed! Check wiring on SDA(19) and SCL(20).");
    }
}

[[noreturn]] static void cbTelemetryTask(void* ) {
    while (1) {
        // Read sensor data using the high-precision mode
        esp_err_t res = sht4x_measure(&sensor, &telemetryValues.temperature, &telemetryValues.humidity);
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
    ESP_LOGI(TAG, "%s", __func__);
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
