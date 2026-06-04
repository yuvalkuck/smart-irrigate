#include "telemetry.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bme680.h"
#include "common_event.h"
#include "esp_event.h"


// Define I2C pins for ESP32-C6
#define I2C_PORT       I2C_NUM_0
#define SDA_GPIO       GPIO_NUM_19
#define SCL_GPIO       GPIO_NUM_20

// BME680 default I2C Address (usually 0x76 or 0x77 depending on SDO pin state)
#define BME680_I2C_ADDR BME680_I2C_ADDR_1
static bme680_t sensor;
static const char* TAG = "BME680:";
bme680_values_float_t telemetryValues;

void init_telemetry(void) {
    ESP_LOGI(TAG, "%s", __func__);
    memset(&sensor, 0, sizeof(bme680_t));
    // Initialize underlying thread-safe esp-idf-lib hardware driver wrapper
    ESP_ERROR_CHECK(i2cdev_init());

    // Connect the sensor descriptor with DFRobot's exact pin infrastructure
    ESP_ERROR_CHECK(bme680_init_desc(&sensor, BME680_I2C_ADDR, I2C_PORT , SDA_GPIO, SCL_GPIO));

    // Perform validation and initialization of chip parameters
    if (bme680_init_sensor(&sensor) != ESP_OK) {
        ESP_LOGE(TAG, "Sensor initialization failed! Check wiring on SDA(19) and SCL(20).");
        return;
    }

    // Configure oversampling profiles
    bme680_set_oversampling_rates(&sensor, BME680_OSR_2X, BME680_OSR_8X, BME680_OSR_1X);

    // Set digital filter size to isolate telemetry spikes
    bme680_set_filter_size(&sensor, BME680_IIR_SIZE_3);

    // Warm up the VOC Gas Sensor Matrix: Target 320°C for 150ms
    //bme680_set_heater_run_program(&sensor, true);
    bme680_set_heater_profile(&sensor, 0, 320, 150);
    bme680_use_heater_profile(&sensor, 0);

    // Set a context environment baseline temperature
    bme680_set_ambient_temperature(&sensor, 25);
}

#define TASK_DELAY 5000

[[noreturn]] static void cbTelemetryTask(void* ) {
    uint32_t duration;
    bme680_get_measurement_duration(&sensor, &duration);
    ESP_LOGI(TAG, "duration:%d", duration);
    while (1) {
        if (bme680_force_measurement(&sensor) == ESP_OK) {
            vTaskDelay(duration);
            if (bme680_get_results_float(&sensor, &telemetryValues) == ESP_OK) {
                esp_event_post(COMMON_BASE_EVENTS, COMMON_EVENT_SENSOR_UPDATED, NULL, 0, portMAX_DELAY);
            }
            else {
                ESP_LOGE(TAG, "Could not fetch registers from BME680.");
            }
        }
        else {
            ESP_LOGE(TAG, "Could not force measurement command.");
        }
        vTaskDelay(pdMS_TO_TICKS(TASK_DELAY));
    }
}

void start_telemetry(void) {
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
