#include <stdio.h>
#include "blesrv.h"
static const char* TAG = "BleSrv:";
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_hs_adv.h"
// #include "host/ble_sm.h"

#include "sdkconfig.h"

// 1. GAP Event Handler (Handles connections/advertising)
static int ble_gap_event_cb(struct ble_gap_event *event, void *arg) {
    return 0;
}

static void start_advertising(void) {
    /* Local variables */
    int rc = 0;
    const char *name = CONFIG_MY_BT_DEVICE_NAME;
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields adv_fields = {0};

    /* Set advertising flags */
    adv_fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    /* Set device name */
    adv_fields.name = (uint8_t *)name;
    adv_fields.name_len = strlen(name);
    adv_fields.name_is_complete = 1;

    /* Set device tx power */
    adv_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    adv_fields.tx_pwr_lvl_is_present = 1;

    // /* Set device appearance */
    // adv_fields.appearance = BLE_GAP_APPEARANCE_GENERIC_TAG;
    // adv_fields.appearance_is_present = 1;
    //
    // /* Set device LE role */
    // adv_fields.le_role = BLE_GAP_LE_ROLE_PERIPHERAL;
    // adv_fields.le_role_is_present = 1;

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

void on_sync_cb(void) {
    start_advertising();
}
void host_task_cb(void *param) {
    nimble_port_run(); // This block keeps the stack running
}


void init_blesrv(void) {
    auto ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init nimble %d ", ret);
        return;
    }
    /* Initialize the NimBLE host configuration. */

    ble_hs_cfg.sync_cb = on_sync_cb;
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

