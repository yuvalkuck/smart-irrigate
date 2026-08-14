#include "telemetry.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "logger.h"
#include "common_event.h"
#include "esp_event.h"
#include "diagnostics.h"
#include "esp_adc/adc_oneshot.h"
#include "sensor_sht4x.h"
#include "sensor_bmp5xx.h"

// Define I2C pins for ESP32-C6
#define I2C_PORT       I2C_NUM_0
#define I2C_SDA_PIN    GPIO_NUM_19
#define I2C_SCL_PIN    GPIO_NUM_20
#define TASK_DELAY     (1000*10)


static const char* TAG = "Telemetry:";

static i2c_master_bus_handle_t master_bus_handler;
static i2c_master_bus_config_t master_bus_config = {
    .i2c_port = I2C_PORT,
    .sda_io_num = I2C_SDA_PIN,
    .scl_io_num = I2C_SCL_PIN,
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .glitch_ignore_cnt = 7
};
///////////////////////////

static SensorSHT4x sensorSHT;
static SensorBMP5xx sensorBMP;

static void i2c_scan(i2c_master_bus_handle_t bus) {
    ESP_LOGI(TAG, "Scanning I2C bus...");
    for (uint8_t addr = 0x03; addr < 0x78; addr++) {
        esp_err_t err = i2c_master_probe(bus, addr, 50);
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "  Found device at 0x%02X", addr);
        }
    }
    ESP_LOGI(TAG, "Scan complete.");
}

esp_err_t init_telemetry() {
    METHODTRACE
    /***** init services ****/
    master_bus_config.flags.enable_internal_pullup = true;
    // I2C Master
    ESP_ERROR_CHECK(i2c_new_master_bus(&master_bus_config, &master_bus_handler));
    // init I2C sensors
    i2c_scan(master_bus_handler);
    auto rc = sensorSHT.init(master_bus_handler);
    if (rc != ESP_OK) {
        ESP_ERROR_CHECK(rc);
        return rc;
    }

    rc = sensorBMP.init(master_bus_handler);
    if (rc != ESP_OK) {
        ESP_ERROR_CHECK(rc);
        return rc;
    }


    auto drc = Diagnostics::execute_operational_self_test(master_bus_handler);
    if (drc != Diagnostics::MaskStateOK::Reset) {
        ESP_LOGE(TAG, "Failed in sensors diagnostic!");
        return ESP_FAIL;
    }
    return ESP_OK;
}

[[noreturn]] static void cbTelemetryTask(void*) {
    TelemetryData telemetryPayload = {0};
    EventData event = {sizeof(TelemetryData), &telemetryPayload};
    while (1) {
        sensorSHT.read(telemetryPayload);
        sensorBMP.read(telemetryPayload);
        esp_event_post(COMMON_BASE_EVENTS, COMMON_EVENT_UPDATED_SENSOR, &event, event.length(), portMAX_DELAY);
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
