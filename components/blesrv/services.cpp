#include "esp_log.h"
#include <host/ble_gatt.h>
#include <host/ble_uuid.h>
#include "inc/services.h"
#include <string>
#include "nvs.h"
#include "sdkconfig.h"

#define NVS_NAMESPACE "."
#define NVS_PARTITION_NAME "config"
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
    ESP_LOGI(TAG, "init_gatt_services");
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
    ESP_LOGI(TAG, "NVS Read Access Callback. Op: %d", ctxt->op);

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        nvs_iterator_t it;
        esp_err_t err;
        std::string nvs_data_str; // String to build the NVS key-value list

        // Iterate through all NVS entries in all namespaces.
        // To iterate a specific namespace, replace the first NULL with the namespace name (e.g., "storage").
        nvs_entry_find(NVS_PARTITION_NAME, NVS_NAMESPACE, NVS_TYPE_ANY, &it);
        while (it != nullptr) {
            nvs_entry_info_t info;
            nvs_entry_info(it, &info); // Get info about the current entry (key, type, namespace)
            nvs_entry_next(&it); // Move to the next entry
            ESP_LOGI(TAG, "NVS Entry: %s, %s, %d", info.key, info.namespace_name, info.type);

            nvs_handle_t nvs_handle;
            // Open NVS handle for the current entry's namespace in read-only mode
            err = nvs_open_from_partition(NVS_PARTITION_NAME, info.namespace_name, NVS_READONLY, &nvs_handle);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "Error (%s) opening NVS namespace %s", esp_err_to_name(err), info.namespace_name);
                continue; // Skip to the next entry if namespace cannot be opened
            }

            // Append key to the string (format: "key:value;")
            nvs_data_str += info.key;
            nvs_data_str += ":";

            // Attempt to read the value based on its type.
            // This example only handles NVS_TYPE_STR and NVS_TYPE_U32.
            // You would need to extend this for other NVS types (e.g., NVS_TYPE_I32, NVS_TYPE_BLOB, etc.).
            if (info.type == NVS_TYPE_STR) {
                size_t required_size;
                err = nvs_get_str(nvs_handle, info.key, NULL, &required_size);
                if (err == ESP_OK) {
                    char* value_str = (char*)malloc(required_size);
                    if (value_str) {
                        err = nvs_get_str(nvs_handle, info.key, value_str, &required_size);
                        if (err == ESP_OK) {
                            nvs_data_str += value_str;
                        }
                        else {
                            ESP_LOGE(TAG, "Error (%s) reading string NVS key %s", esp_err_to_name(err), info.key);
                            nvs_data_str += "<str_read_error>";
                        }
                        free(value_str);
                    }
                    else {
                        ESP_LOGE(TAG, "Failed to allocate memory for NVS string value");
                        nvs_data_str += "<mem_alloc_error>";
                    }
                }
                else {
                    ESP_LOGE(TAG, "Error (%s) getting size for string NVS key %s", esp_err_to_name(err), info.key);
                    nvs_data_str += "<str_size_error>";
                }
            }
            else if (info.type == NVS_TYPE_U32) {
                uint32_t value_u32;
                err = nvs_get_u32(nvs_handle, info.key, &value_u32);
                if (err == ESP_OK) {
                    nvs_data_str += std::to_string(value_u32);
                }
                else {
                    ESP_LOGE(TAG, "Error (%s) reading u32 NVS key %s", esp_err_to_name(err), info.key);
                    nvs_data_str += "<u32_read_error>";
                }
            }
            else {
                // For unhandled types, just indicate the type
                nvs_data_str += "<type:";
                nvs_data_str += std::to_string(info.type);
                nvs_data_str += ">";
            }

            nvs_data_str += ";"; // Separator for key-value pairs

            nvs_close(nvs_handle); // Close the NVS handle for the current namespace
        }

        // IMPORTANT CONSIDERATION: Response Size Limitation
        // The maximum length of a characteristic value is limited by the negotiated MTU.
        // The default MTU is typically 23 bytes, meaning only 20 bytes of actual data
        // can be sent in a single packet (23 - 3 bytes for ATT header).
        // If the 'nvs_data_str' is longer than the MTU, it will be truncated,
        // or the operation might fail.
        //
        // For a robust solution with potentially large NVS data, consider:
        // 1. Increasing the MTU (if supported by both devices and configured in menuconfig).
        // 2. Implementing a mechanism to send data in chunks (e.g., using notifications
        //    or multiple read requests with an offset).
        // 3. Providing a separate characteristic for "get NVS value by key" to retrieve
        //    individual values rather than the entire list at once.
        ESP_LOGW(TAG, "nvs_data_str len: %d", nvs_data_str.size());
        if (nvs_data_str.length() > (BLE_ATT_MTU_DFLT - 3)) {
            ESP_LOGW(TAG, "NVS data string (%zu bytes) is larger than default MTU (%d bytes). "
                     "The response might be truncated. Consider increasing MTU or implementing chunking.",
                     nvs_data_str.length(), BLE_ATT_MTU_DFLT - 3);
        }

        // Copy the NVS data string to the GATT response buffer (os_mbuf)
        err = os_mbuf_append(ctxt->om, nvs_data_str.c_str(), nvs_data_str.length());
        if (err != 0) {
            ESP_LOGE(TAG, "Failed to append NVS data to mbuf; rc=%d", err);
            return BLE_ATT_ERR_INSUFFICIENT_RES; // Indicate resource error
        }
    }
    return 0; // Success
}

static int nvs_write_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt* ctxt, void* arg) {
    return 0;
}
