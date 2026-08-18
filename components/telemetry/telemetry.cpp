#include "telemetry.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "logger.h"
#include "common_event.h"
#include "esp_event.h"
#include "esp_adc/adc_oneshot.h"
#include "gpio_declaraion.h"
#include "driver/i2c_master.h"
/*###*/
#include "sensor_sht4x.h"
#include "sensor_bmp5xx.h"
#include "sensor_ds18b20.h"
#include "sensor_wind.h"
#include "sensor_xdb401.h"
#include <bitset>

// Define I2C pins for ESP32-C6
#define TASK_DELAY     (1000*10)

namespace Diagnostics {
    enum MaskStateOK {
        NoValue = 0x00,
        SHT41 = (1 << 0),
        BMP581 = (1 << 1),
        DS18B20 = (1 << 2),
        XDB401 = (1 << 3),
        WindSpeed = (1 << 4),
        TSL2591 = (1 << 5),
        All = SHT41 | BMP581 | TSL2591 | DS18B20 | XDB401 | WindSpeed
    };

    constexpr MaskStateOK operator|(MaskStateOK lhs, MaskStateOK rhs) {
        return static_cast<MaskStateOK>(
            static_cast<std::underlying_type_t<MaskStateOK>>(lhs) |
            static_cast<std::underlying_type_t<MaskStateOK>>(rhs)
        );
    }

    constexpr MaskStateOK& operator|=(MaskStateOK& lhs, MaskStateOK rhs) {
        lhs = lhs | rhs;
        return lhs;
    }

    // Declaration only: Tells the compiler this function exists globally
}

static constexpr auto TAG = "Telemetry:";
adc_oneshot_unit_handle_t adc_handle;
onewire_bus_handle_t onewire_bus_handle;
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
static SensorDS18B20 sensorDS18B20;
static SensorWind sensorWind;
static SensorXDB4xx sensorXDB4xx;

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

#define ONLINE_STATE(trg,hand,clss,stat) trg |= clss.online(hand) ? Diagnostics::MaskStateOK::stat : Diagnostics::MaskStateOK::NoValue
#define INIT_SENSOR(hand,obj,stat)    if ( state == (state | Diagnostics::MaskStateOK::stat)) { \
    auto rc = obj.init(hand);  \
    if (rc != ESP_OK) { ESP_LOGE(TAG, "Sensor Init Failed: %s (%s)", # stat, esp_err_to_name(rc)); return rc;} \
} else {ESP_LOGE(TAG, "Sensor Offline: %s", # stat); return ESP_FAIL; }


esp_err_t init_telemetry() {
    METHODTRACE
    /***** init services ****/
    master_bus_config.flags.enable_internal_pullup = true;
    // I2C Master
    ESP_ERROR_CHECK(i2c_new_master_bus(&master_bus_config, &master_bus_handler));
    // init I2C sensors
    i2c_scan(master_bus_handler);
    Diagnostics::MaskStateOK state = Diagnostics::MaskStateOK::NoValue;
    ONLINE_STATE(state, master_bus_handler, sensorSHT, SHT41);
    ONLINE_STATE(state, master_bus_handler, sensorBMP, BMP581);
    // ONLINE_STATE(state,master_bus_handler, sensorTSL25xx, TSL2591); - Disable until driver will be available
    ONLINE_STATE(state, onewire_bus_handle, sensorDS18B20, DS18B20);
    ONLINE_STATE(state, adc_handle, sensorXDB4xx, XDB401);
    ONLINE_STATE(state, adc_handle, sensorWind, WindSpeed);
    const std::bitset<6> pr = static_cast<uint8_t>(state);
    ESP_LOGI(TAG, "Sensors Online:%s", pr.to_string().c_str());
    //-----------------------------
    INIT_SENSOR(master_bus_handler,sensorSHT,SHT41)
    INIT_SENSOR(master_bus_handler,sensorBMP,BMP581)
    // INIT_SENSOR(master_bus_handler,sensorTSL25xx,TSL2591) - Disable until driver will be available
    INIT_SENSOR(onewire_bus_handle,sensorDS18B20,DS18B20)
    INIT_SENSOR(adc_handle,sensorXDB4xx,XDB401)
    INIT_SENSOR(adc_handle,sensorWind,WindSpeed)

    return ESP_OK;
}

[[noreturn]] static void cbTelemetryTask(void*) {
    TelemetryData telemetryPayload = {0};
    EventData event = {sizeof(TelemetryData), &telemetryPayload};
    while (1) {
        sensorSHT.read(telemetryPayload);
        sensorBMP.read(telemetryPayload);
        sensorXDB4xx.read(telemetryPayload);
        sensorDS18B20.read(telemetryPayload);
        sensorWind.read(telemetryPayload);
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
        nullptr,
        5,
        nullptr
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create task due to insufficient memory!");
    }
}
