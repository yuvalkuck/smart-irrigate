#include "esp_log.h"
#include <host/ble_gatt.h>
#include <host/ble_uuid.h>
#include "inc/services.h"
// NVS includes

#include "nvs.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

static const char* TAG = "BleHandler:";
// Characteristic UUID for NVS Read (to get a list of key-value pairs).
// Example Characteristic UUID: 00000002-8D6A-46F6-B20C-9A5163000000
static const ble_uuid128_t gatt_svr_svc_nvs_uuid =
    BLE_UUID128_INIT(0x7e, 0xa2, 0x22, 0x2d, 0xb2, 0x1a, 0xce, 0x9f,
                     0xa7, 0x44, 0x30, 0x1a, 0x59, 0x16, 0xa8, 0x88);
static const ble_uuid128_t gatt_svr_chr_nvs_read_uuid =
    BLE_UUID128_INIT(0x40, 0xb4, 0x05, 0xd4, 0x4a, 0x10, 0xef, 0xb9,
                     0x5a, 0x4d, 0x9d, 0x04, 0x7d, 0xfb, 0x4d, 0xf7);
static const ble_uuid128_t gatt_svr_chr_nvs_write_uuid =
    BLE_UUID128_INIT(0x69, 0x50, 0x6b, 0x22, 0xe4, 0x3f, 0x79, 0x97,
                     0xbe, 0x4b, 0x6a, 0xd4, 0xce, 0xb7, 0xcb, 0x6d);
// Forward declaration for the characteristic access callback
static int nvs_read_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt* ctxt, void* arg);

static int nvs_write_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt* ctxt, void* arg);


// --- GATT Service Definitions ---
// This array defines all custom GATT services provided by your device.
// The GAP service is typically initialized separately by ble_svc_gap_init().
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        // NVS Service Definition
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_nvs_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]){
            {
                // NVS Read Characteristic Definition
                .uuid = &gatt_svr_chr_nvs_read_uuid.u,
                .access_cb = nvs_read_access_cb, // The callback function to handle read requests
                .flags = BLE_GATT_CHR_F_READ, // This characteristic is readable
            },
            {
                // NVS Read Characteristic Definition
                .uuid = &gatt_svr_chr_nvs_write_uuid.u,
                .access_cb = nvs_write_access_cb, // The callback function to handle read requests
                .flags = BLE_GATT_CHR_F_WRITE, // This characteristic is writable
            },
            {
                0, // Zero-fill to terminate characteristics array
            }
        },
    },
    {
        0, // Zero-fill to terminate services array
    }
};

int init_gatt_services() {
    // Register the custom GATT services defined in gatt_svr_svcs
    int rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_count_cfg failed: %d", rc);
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_add_svcs failed: %d", rc);
        return rc;
    }
    return 0;
}
//////////////////////////////////
static int nvs_read_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                              struct ble_gatt_access_ctxt* ctxt, void* arg) {
    return 0;
}

static int nvs_write_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt* ctxt, void* arg) {
    return 0;
}
