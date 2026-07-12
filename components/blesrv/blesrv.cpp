#include "blesrv.h"

#include <cstring>

#include "esp_bt.h"
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_hs_adv.h"
#include "services/gap/ble_svc_gap.h"
#include "sdkconfig.h"

#include "../flash/include/flash.h"
#include "inc/services.h"

extern const char* events[];
static const char* TAG = "BleSrv:";
#define GATT_SVR_SVC_ALERT_UUID               0x1811

static uint8_t own_addr_type = BLE_OWN_ADDR_RANDOM;
#define BLE_SVC_GAP_APPEARANCE_GEN_DISPLAY 0x0100
#define BLE_SVC_GAP_APPEARANCE_GEN_THERMOMETER 0x0300;
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x01
// Characteristic UUID for NVS Read (to get a list of key-value pairs).
// Example Characteristic UUID: 00000002-8D6A-46F6-B20C-9A5163000000
static void start_advertising();
static void
print_addr(const void* addr) {
    const char* u8p = (const char*)addr;

    MODLOG_DFLT(INFO, "%02x:%02x:%02x:%02x:%02x:%02x",
                u8p[5], u8p[4], u8p[3], u8p[2], u8p[1], u8p[0]);
}

static void
bleprph_print_conn_desc(struct ble_gap_conn_desc* desc) {
    MODLOG_DFLT(INFO, "handle=%d our_ota_addr_type=%d our_ota_addr=",
                desc->conn_handle, desc->our_ota_addr.type);
    print_addr(desc->our_ota_addr.val);
    MODLOG_DFLT(INFO, " our_id_addr_type=%d our_id_addr=",
                desc->our_id_addr.type);
    print_addr(desc->our_id_addr.val);
    MODLOG_DFLT(INFO, " peer_ota_addr_type=%d peer_ota_addr=",
                desc->peer_ota_addr.type);
    print_addr(desc->peer_ota_addr.val);
    MODLOG_DFLT(INFO, " peer_id_addr_type=%d peer_id_addr=",
                desc->peer_id_addr.type);
    print_addr(desc->peer_id_addr.val);
    MODLOG_DFLT(INFO, " conn_itvl=%d conn_latency=%d supervision_timeout=%d "
                "encrypted=%d authenticated=%d bonded=%d\n",
                desc->conn_itvl, desc->conn_latency,
                desc->supervision_timeout,
                desc->sec_state.encrypted,
                desc->sec_state.authenticated,
                desc->sec_state.bonded);
}

// 1. GAP Event Handler (Handles connections/advertising)
static int ble_gap_event_cb(struct ble_gap_event* event, void* arg) {
    const char* event_name = (event->type < 64 && events[event->type])
                              ? events[event->type] : "UNKNOWN";
    ESP_LOGI(TAG, "ble_gap_event_cb, event: %d : %s", event->type, event_name);

    struct ble_gap_conn_desc desc;
    int rc;

    switch (event->type) {
        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(TAG, "adv complete, reason=%d", event->adv_complete.reason);
            // Only restart if not stopped by a connection
            if (event->adv_complete.reason != BLE_HS_ENOTCONN) {
                start_advertising();
            }
            break;
        case BLE_GAP_EVENT_CONNECT:
            /* A new connection was established or a connection attempt failed. */
            MODLOG_DFLT(INFO, "connection %s; status=%d ",
                        event->connect.status == 0 ? "established" : "failed",
                        event->connect.status);
            if (event->connect.status == 0) {
                rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
                assert(rc == 0);
                bleprph_print_conn_desc(&desc);
            }
            MODLOG_DFLT(INFO, "\n");

            if (event->connect.status != 0) {
                /* Connection failed; resume advertising. */
                start_advertising();
            }
            break;
        case BLE_GAP_EVENT_LINK_ESTAB:
            ESP_LOGI(TAG, "Device connection success");
            break;
        case BLE_GAP_EVENT_DATA_LEN_CHG: {
            // Access the data_len_chg structure inside the event union
            const auto& data_len = event->data_len_chg;

            ESP_LOGI(TAG, "Link Layer Data Length Changed!");
            ESP_LOGI(TAG, "Connection Handle : %d", data_len.conn_handle);

            // Maximum TX octets and time the local device will support
            ESP_LOGI(TAG, "Max TX Octets     : %u bytes", data_len.max_tx_octets);
            ESP_LOGI(TAG, "Max TX Time       : %u us", data_len.max_tx_time);

            // Maximum RX octets and time the local device will support
            ESP_LOGI(TAG, "Max RX Octets     : %u bytes", data_len.max_rx_octets);
            ESP_LOGI(TAG, "Max RX Time       : %u us", data_len.max_rx_time);
        }
        break;
        case BLE_GAP_EVENT_CONN_UPDATE_REQ: {
            // Access the connection update request parameters
            const auto& req = event->conn_update_req;

            // Access the self-pointing status parameters
            auto* signall = event->conn_update_req.peer_params;

            ESP_LOGI(TAG, "Peer requested Connection Parameter Update!");
            ESP_LOGI(TAG, "Connection Handle : %d", req.conn_handle);

            // Connection interval: time between two consecutive connection events (Units: 1.25ms)
            ESP_LOGI(TAG, "Min Interval      : %u (~%.2f ms)", signall->itvl_min, signall->itvl_min * 1.25);
            ESP_LOGI(TAG, "Max Interval      : %u (~%.2f ms)", signall->itvl_max, signall->itvl_max * 1.25);

            // Peripheral Latency: Number of connection events the peripheral can skip
            ESP_LOGI(TAG, "Latency           : %u events", signall->latency);

            // Supervision Timeout: Time before connection is considered lost (Units: 10ms)
            ESP_LOGI(TAG, "Timeout           : %u (~%u ms)", signall->supervision_timeout,
                     signall->supervision_timeout * 10);
            // Return 0 to indicate acceptance.
            ESP_LOGI(TAG, "Accepting peer connection parameters.");
        }
        break;

        case BLE_GAP_EVENT_DISCONNECT:
            MODLOG_DFLT(INFO, "disconnect; reason=%d ", event->disconnect.reason);
            bleprph_print_conn_desc(&event->disconnect.conn);
            MODLOG_DFLT(INFO, "\n");
            /* Always resume advertising after a disconnect. */
            start_advertising();
            break;
        default:
            ESP_LOGW(TAG, "UNHANDLED EVENT %d : %s", event->type, event_name);
    }

    return 0;
}

static struct ble_gap_adv_params adv_params = {};
static struct ble_hs_adv_fields adv_fields = {};
static struct ble_npl_callout adv_callout;

static void adv_callout_cb(struct ble_npl_event* ev) {
    ESP_LOGI(TAG, "adv_callout_cb: adv_active=%d, gap_conn_active=%d",
                 ble_gap_adv_active(),
                 ble_gap_conn_active());
    start_advertising();
}

static void start_advertising() {
    // WITH:
    if (ble_gap_adv_active()) {
        ble_gap_adv_stop();
    }
    if (ble_gap_disc_active()) {
        ble_gap_disc_cancel();
    }
    int rc = 0;
    NvsConfig hCfg;
    static char buff[32] = {0};

    if (!hCfg.getStr(CFG_NVS_KEY_BT_DEVICE_NAME, buff, sizeof(buff))) {
        ESP_LOGE(TAG, "Failed to get BT device name");
        strncpy(buff, "ESP32C6_BLE", sizeof(buff) - 1);
    }
    if (strlen(buff) > 19) {
        buff[19] = '\0';
    }

    ESP_LOGI(TAG, ">>>>>>> start advertising [%s] (%d bytes)", buff, strlen(buff));

    memset(&adv_fields, 0, sizeof(adv_fields));
    adv_fields.flags                 = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    adv_fields.name                  = (uint8_t*)buff;
    adv_fields.name_len              = strlen(buff);
    adv_fields.name_is_complete      = 1;
    adv_fields.tx_pwr_lvl            = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    adv_fields.tx_pwr_lvl_is_present = 1;
    adv_fields.appearance            = BLE_SVC_GAP_APPEARANCE_GEN_DISPLAY;
    adv_fields.appearance_is_present = 1;

    rc = ble_gap_adv_set_fields(&adv_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv_set_fields failed: %d, retrying...", rc);
        // ↓ retry via callout instead of giving up
        ble_npl_callout_reset(&adv_callout, pdMS_TO_TICKS(100));
        return;
    }

    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(BLE_OWN_ADDR_RANDOM, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event_cb, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv_start failed: %d adv=%d conn=%d disc=%d",
                 rc,
                 ble_gap_adv_active(),
                 ble_gap_conn_active(),
                 ble_gap_disc_active());
        ble_npl_callout_reset(&adv_callout, pdMS_TO_TICKS(100));
        return;
    }
    ESP_LOGI(TAG, "advertising started OK");
}
// static void start_advertising() {
//     /* Local variables */
//     int rc = 0;
//     NvsConfig hCfg;
//     static char buff[32] = {0};
//     if ( !hCfg.getStr(CFG_NVS_KEY_BT_DEVICE_NAME, buff, sizeof(buff)) ) {
//         ESP_LOGE(TAG, "Failed to get BT device name");
//         strncpy(buff, "ESP32C6_BLE", sizeof(buff) - 1);
//     }
//     if (strlen(buff) > 19) {
//         ESP_LOGW(TAG, "Device name too long, truncating");
//         buff[19] = '\0';
//     }
//
//     ESP_LOGI(TAG, ">>>>>>> start advertising [%s] (%d bytes)", buff, strlen(buff));
//     /* Set advertising flags */
//     adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
//
//     /* Set device name */
//     adv_fields.name = (uint8_t*)buff;
//     adv_fields.name_len = strlen(buff);
//     adv_fields.name_is_complete = 1;
//
//     //CONFIG_MY_BT_DEVICE_UUID
//
//     /* Set device tx power */
//     adv_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
//     adv_fields.tx_pwr_lvl_is_present = 1;
//
//     // /* Set device appearance */
//     adv_fields.appearance = BLE_SVC_GAP_APPEARANCE_GEN_DISPLAY;
//     adv_fields.appearance_is_present = 1;
//     //
//     // /* Set device LE role */
//     adv_fields.le_role = BLE_GAP_LE_ROLE_PERIPHERAL;
//     adv_fields.le_role_is_present = 1;
//
//     /* Set advertisement fields */
//     rc = ble_gap_adv_set_fields(&adv_fields);
//     if (rc != 0) {
//         ESP_LOGE(TAG, "failed to set advertising data, error code: %d", rc);
//         return;
//     }
//     memset(&adv_params, 0, sizeof(adv_params));
//     adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; // Undirected connectable
//     adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // General discoverable
//     ble_gap_adv_start(BLE_OWN_ADDR_RANDOM, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event_cb, NULL);
// }

static void
on_reset_cb(int reason) {
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

static void on_sync_cb() {
    ESP_LOGI(TAG, "on_sync_cb");
    /* Figure out address to use while advertising (no privacy for now) */
    auto rc = ble_hs_id_infer_auto(1, &own_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }
    NvsConfig hCfg;
    char buff[64] = {0};
    if ( !hCfg.getStr(CFG_NVS_KEY_BT_DEVICE_NAME, buff, sizeof(buff)) ) {
        ESP_LOGE(TAG, "Failed to get BT device name");
        strncpy(buff, "ESP32C6_BLE_Device", sizeof(buff) - 1);
    }

    // Set your device name
    ble_svc_gap_device_name_set(buff);

    /* Printing ADDR */
    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "ble_hs_id_copy_addr; rc=%d\n", rc);
        return;
    }

    MODLOG_DFLT(INFO, "Device Address: ");
    print_addr(addr_val);
    MODLOG_DFLT(INFO, "\n");

    ble_npl_callout_reset(&adv_callout, pdMS_TO_TICKS(3000));
    // ble_npl_callout_init(&adv_callout, nimble_port_get_dflt_eventq(), adv_callout_cb, NULL);
    // ble_npl_callout_reset(&adv_callout, pdMS_TO_TICKS(500));
}

void host_task_cb(void* param) {
    ESP_LOGI(TAG, "host_task_cb");
    nimble_port_run(); // This block keeps the stack running
}

void gatt_svr_register_cb(struct ble_gatt_register_ctxt* ctxt, void* arg) {
    ESP_LOGI(TAG, "gatt_svr_register_cb");
}

void init_blesrv() {
    auto ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init nimble %d ", ret);
        return;
    }
    ble_npl_callout_init(&adv_callout, nimble_port_get_dflt_eventq(), adv_callout_cb, NULL);
    /* Initialize the NimBLE host configuration. */

    ble_hs_cfg.sync_cb = on_sync_cb;
    ble_hs_cfg.reset_cb = on_reset_cb;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.sm_sc = 0;
    ret = init_gatt_services();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init gatt_services %d ", ret);
    }
}

void start_blesrv() {
    nimble_port_freertos_init(host_task_cb);
}