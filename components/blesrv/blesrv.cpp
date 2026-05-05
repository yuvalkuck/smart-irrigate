#include "blesrv.h"

#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_hs_adv.h"
#include "services/gap/ble_svc_gap.h"
#include "sdkconfig.h"


//#include "host/ble_sm.h"

static const char* TAG = "BleSrv:";
#define BLE_ATT_UUID_PRIMARY_SERVICE 0x2800
#define GATT_SVR_SVC_ALERT_UUID               0x1811

// 1. GAP Event Handler (Handles connections/advertising)
static int ble_gap_event_cb(struct ble_gap_event* event, void* arg) {
    ESP_LOGI(TAG, "ble_gap_event_cb");
    return 0;
}
static uint8_t own_addr_type = BLE_OWN_ADDR_RANDOM;
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x01
static void
print_addr(const void *addr)
{
    const char* u8p = (const char *)addr;

    MODLOG_DFLT(INFO, "%02x:%02x:%02x:%02x:%02x:%02x",
                u8p[5], u8p[4], u8p[3], u8p[2], u8p[1], u8p[0]);
}

static void start_advertising(void) {
    /* Local variables */
    int rc = 0;
    const char* name = CONFIG_MY_BT_DEVICE_NAME;
    struct ble_gap_adv_params adv_params = {};
    struct ble_hs_adv_fields adv_fields = {};
    ESP_LOGI(TAG, ">>>>>>> start advertising");
    /* Set advertising flags */
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    /* Set device name */
    adv_fields.name = (uint8_t*)name;
    adv_fields.name_len = strlen(name);
    adv_fields.name_is_complete = 1;

      //CONFIG_MY_BT_DEVICE_UUID

    /* Set device tx power */
    adv_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    adv_fields.tx_pwr_lvl_is_present = 1;

    // /* Set device appearance */
    adv_fields.appearance = BLE_SVC_GAP_APPEARANCE_GEN_COMPUTER;
    adv_fields.appearance_is_present = 1;
    //
    // /* Set device LE role */
    adv_fields.le_role = BLE_GAP_LE_ROLE_PERIPHERAL;
    adv_fields.le_role_is_present = 1;

    /* Set advertiement fields */
    rc = ble_gap_adv_set_fields(&adv_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to set advertising data, error code: %d", rc);
        return;
    }
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND; // Undirected connectable
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN; // General discoverable
    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event_cb, NULL);
}

static void
on_reset_cb(int reason)
{
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

static void on_sync_cb(void) {
    ESP_LOGI(TAG, "on_sync_cb");
    /* Figure out address to use while advertising (no privacy for now) */
    auto rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    // Set your device name
    ble_svc_gap_device_name_set(CONFIG_MY_BT_DEVICE_NAME);

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

    start_advertising();
}

void host_task_cb(void* param) {
    ESP_LOGI(TAG, "host_task_cb");
    nimble_port_run(); // This block keeps the stack running
}

void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
    ESP_LOGI(TAG, "gatt_svr_register_cb");
}

void init_blesrv(void) {
    auto ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init nimble %d ", ret);
        return;
    }
    /* Initialize the NimBLE host configuration. */

    ble_hs_cfg.sync_cb = on_sync_cb;
    ble_hs_cfg.reset_cb = on_reset_cb;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.sm_sc = 0;

    //
    //
    // #ifdef CONFIG_EXAMPLE_BONDING
    //     ble_hs_cfg.sm_bonding = 1;
    //     /* Enable the appropriate bit masks to make sure the keys
    //      * that are needed are exchanged
    //      */
    //     ble_hs_cfg.sm_our_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC;
    //     ble_hs_cfg.sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC;
    // #endif
    // #ifdef CONFIG_EXAMPLE_MITM
    //     ble_hs_cfg.sm_mitm = 1;
    // #endif
    // #ifdef CONFIG_EXAMPLE_USE_SC
    //     ble_hs_cfg.sm_sc = 1;
    // #else
    //     ble_hs_cfg.sm_sc = 0;
    // #endif
    // #ifdef CONFIG_EXAMPLE_RESOLVE_PEER_ADDR
    //     /* Stores the IRK */
    //     ble_hs_cfg.sm_our_key_dist |= BLE_SM_PAIR_KEY_DIST_ID;
    //     ble_hs_cfg.sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ID;
    // #endif
    //     // ble_sm_configure_static_passkey(CONFIG_MY_BT_PASSWORD , true);

}

void start_blesrv(void) {
    nimble_port_freertos_init(host_task_cb);
}
