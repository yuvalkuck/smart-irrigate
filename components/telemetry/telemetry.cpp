#include "telemetry.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bme680.h"


// Define I2C pins for ESP32-C6
#define I2C_PORT       I2C_NUM_0
#define SDA_GPIO       GPIO_NUM_19
#define SCL_GPIO       GPIO_NUM_20

// BME680 default I2C Address (usually 0x76 or 0x77 depending on SDO pin state)
#define BME680_I2C_ADDR BME680_I2C_ADDR_1
static bme680_t sensor;
static const char* TAG = "BME680:";

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
        vTaskDelete(NULL);
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

void start_telemetry(void) {
    bme680_values_float_t values;
    uint32_t duration;

    // Get time required for a reading cycle
    bme680_get_measurement_duration(&sensor, &duration);

    while (1) {
        // Run forced evaluation pass
        if (bme680_force_measurement(&sensor) == ESP_OK) {
            // Block cleanly while hardware state machine executes the conversion
            vTaskDelay(duration / portTICK_PERIOD_MS);

            // Fetch processed values
            if (bme680_get_results_float(&sensor, &values) == ESP_OK) {
                // Print everything directly because the library ensures valid float parsing on ESP_OK
                ESP_LOGI(TAG, "Temp: %.2f °C | Humidity: %.2f %% | Pressure: %.2f hPa",
                         values.temperature, values.humidity, values.pressure);

                ESP_LOGI(TAG, "Gas Resistance: %.2f Ohm", values.gas_resistance);
                printf("----------------------------------------------------------------\n");
            }
            else {
                ESP_LOGE(TAG, "Could not fetch registers from BME680.");
            }
        }
        else {
            ESP_LOGE(TAG, "Could not force measurement command.");
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
