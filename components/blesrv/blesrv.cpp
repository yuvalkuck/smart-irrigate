#include "blesrv.h"

#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_hs_adv.h"
#include "services/gap/ble_svc_gap.h"
#include "sdkconfig.h"
#include <unordered_map>

extern const char *events[];

//#include "host/ble_sm.h"

static const char* TAG = "BleSrv:";
#define BLE_ATT_UUID_PRIMARY_SERVICE 0x2800
#define GATT_SVR_SVC_ALERT_UUID               0x1811

static uint8_t own_addr_type = BLE_OWN_ADDR_RANDOM;
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x01

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
    ESP_LOGI(TAG, "ble_gap_event_cb, event: %d : %s", event->type, events[event->type]);
#if NIMBLE_BLE_CONNECT
    struct ble_gap_conn_desc desc;
    int rc;
#endif

    switch (event->type) {
#if NIMBLE_BLE_CONNECT
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
            // #if CONFIG_EXAMPLE_EXTENDED_ADV
            //             ext_bleprph_advertise();
            // #else
            //             bleprph_advertise();
            // #endif
        }

#if MYNEWT_VAL(BLE_POWER_CONTROL)
        bleprph_power_control(event->connect.conn_handle);
#endif
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        MODLOG_DFLT(INFO, "disconnect; reason=%d ", event->disconnect.reason);
        bleprph_print_conn_desc(&event->disconnect.conn);
        MODLOG_DFLT(INFO, "\n");

        /* Connection terminated; resume advertising. */
        // #if CONFIG_EXAMPLE_EXTENDED_ADV
        //         ext_bleprph_advertise();
        // #else
        //         bleprph_advertise();
        // #endif
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE:
        /* The central has updated the connection parameters. */
        MODLOG_DFLT(INFO, "connection updated; status=%d ",
                    event->conn_update.status);
        rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        assert(rc == 0);
        bleprph_print_conn_desc(&desc);
        MODLOG_DFLT(INFO, "\n");
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        MODLOG_DFLT(INFO, "advertise complete; reason=%d",
                    event->adv_complete.reason);
        // #if CONFIG_EXAMPLE_EXTENDED_ADV
        //         ext_bleprph_advertise();
        // #else
        //         bleprph_advertise();
        // #endif
        return 0;

    case BLE_GAP_EVENT_ENC_CHANGE:
        /* Encryption has been enabled or disabled for this connection. */
        MODLOG_DFLT(INFO, "encryption change event; status=%d ",
                    event->enc_change.status);
        rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
        assert(rc == 0);
        bleprph_print_conn_desc(&desc);
        MODLOG_DFLT(INFO, "\n");
        return 0;

    case BLE_GAP_EVENT_NOTIFY_TX:
        MODLOG_DFLT(INFO, "notify_tx event; conn_handle=%d attr_handle=%d "
                    "status=%d is_indication=%d",
                    event->notify_tx.conn_handle,
                    event->notify_tx.attr_handle,
                    event->notify_tx.status,
                    event->notify_tx.indication);
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        MODLOG_DFLT(INFO, "subscribe event; conn_handle=%d attr_handle=%d "
                    "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
                    event->subscribe.conn_handle,
                    event->subscribe.attr_handle,
                    event->subscribe.reason,
                    event->subscribe.prev_notify,
                    event->subscribe.cur_notify,
                    event->subscribe.prev_indicate,
                    event->subscribe.cur_indicate);
        return 0;

    case BLE_GAP_EVENT_MTU:
        MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.channel_id,
                    event->mtu.value);
        return 0;

    case BLE_GAP_EVENT_REPEAT_PAIRING:
        /* We already have a bond with the peer, but it is attempting to
         * establish a new secure link.  This app sacrifices security for
         * convenience: just throw away the old bond and accept the new link.
         */

        /* Delete the old bond. */
        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == 0);
        ble_store_util_delete_peer(&desc.peer_id_addr);

        /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
         * continue with the pairing operation.
         */
        return BLE_GAP_REPEAT_PAIRING_RETRY;

    case BLE_GAP_EVENT_PASSKEY_ACTION:
        ESP_LOGI(TAG, "PASSKEY_ACTION_EVENT started");
        struct ble_sm_io pkey = {0};
        int key = 0;

        if (event->passkey.params.action == BLE_SM_IOACT_DISP) {
            pkey.action = event->passkey.params.action;
            pkey.passkey = 123456; // This is the passkey to be entered on peer
            ESP_LOGI(TAG, "Enter passkey %" PRIu32 "on the peer side", pkey.passkey);
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            ESP_LOGI(TAG, "ble_sm_inject_io result: %d", rc);
        }
        else if (event->passkey.params.action == BLE_SM_IOACT_NUMCMP) {
            ESP_LOGI(TAG, "Passkey on device's display: %" PRIu32, event->passkey.params.numcmp);
            ESP_LOGI(TAG, "Accept or reject the passkey through console in this format -> key Y or key N");
            pkey.action = event->passkey.params.action;
            // if (scli_receive_key(&key)) {
            //     pkey.numcmp_accept = key;
            // } else {
            //     pkey.numcmp_accept = 0;
            //     ESP_LOGE(TAG, "Timeout! Rejecting the key");
            // }
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            ESP_LOGI(TAG, "ble_sm_inject_io result: %d", rc);
        }
        else if (event->passkey.params.action == BLE_SM_IOACT_OOB) {
            static uint8_t tem_oob[16] = {0};
            pkey.action = event->passkey.params.action;
            for (int i = 0; i < 16; i++) {
                pkey.oob[i] = tem_oob[i];
            }
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            ESP_LOGI(TAG, "ble_sm_inject_io result: %d", rc);
        }
        else if (event->passkey.params.action == BLE_SM_IOACT_INPUT) {
            ESP_LOGI(TAG, "Enter the passkey through console in this format-> key 123456");
            pkey.action = event->passkey.params.action;
            // if (scli_receive_key(&key)) {
            //     pkey.passkey = key;
            // } else {
            //     pkey.passkey = 0;
            //     ESP_LOGE(TAG, "Timeout! Passing 0 as the key");
            // }
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            ESP_LOGI(TAG, "ble_sm_inject_io result: %d", rc);
        }
        return 0;
    // case BLE_GAP_EVENT_AUTHORIZE:
    //     MODLOG_DFLT(INFO, "authorize event: conn_handle=%d attr_handle=%d is_read=%d",
    //                 event->authorize.conn_handle,
    //                 event->authorize.attr_handle,
    //                 event->authorize.is_read);
    //
    //     /* The default behaviour for the event is to reject authorize request */
    //     event->authorize.out_response = BLE_GAP_AUTHORIZE_REJECT;
    //     return 0;

#if MYNEWT_VAL(BLE_POWER_CONTROL)
    case BLE_GAP_EVENT_TRANSMIT_POWER:
        MODLOG_DFLT(INFO, "Transmit power event : status=%d conn_handle=%d reason=%d "
                    "phy=%d power_level=%x power_level_flag=%d delta=%d",
                    event->transmit_power.status,
                    event->transmit_power.conn_handle,
                    event->transmit_power.reason,
                    event->transmit_power.phy,
                    event->transmit_power.transmit_power_level,
                    event->transmit_power.transmit_power_level_flag,
                    event->transmit_power.delta);
        return 0;

    case BLE_GAP_EVENT_PATHLOSS_THRESHOLD:
        MODLOG_DFLT(INFO, "Pathloss threshold event : conn_handle=%d current path loss=%d "
                    "zone_entered =%d",
                    event->pathloss_threshold.conn_handle,
                    event->pathloss_threshold.current_path_loss,
                    event->pathloss_threshold.zone_entered);
        return 0;
#endif

#if MYNEWT_VAL(BLE_EATT_CHAN_NUM) > 0
    case BLE_GAP_EVENT_EATT:
        MODLOG_DFLT(INFO, "EATT %s : conn_handle=%d cid=%d",
                    event->eatt.status ? "disconnected" : "connected",
                    event->eatt.conn_handle,
                    event->eatt.cid);
        if (event->eatt.status) {
            /* Abort if disconnected */
            return 0;
        }
        cids[bearers] = event->eatt.cid;
        bearers += 1;
        if (bearers != MYNEWT_VAL(BLE_EATT_CHAN_NUM)) {
            /* Wait until all EATT bearers are connected before proceeding */
            return 0;
        }
        /* Set the default bearer to use for further procedures */
        rc = ble_att_set_default_bearer_using_cid(event->eatt.conn_handle, cids[0]);
        if (rc != 0) {
            MODLOG_DFLT(INFO, "Cannot set default EATT bearer, rc = %d\n", rc);
            return rc;
        }

        return 0;
#endif

#if MYNEWT_VAL(BLE_CONN_SUBRATING)
    case BLE_GAP_EVENT_SUBRATE_CHANGE:
        MODLOG_DFLT(INFO, "Subrate change event : conn_handle=%d status=%d factor=%d",
                    event->subrate_change.conn_handle,
                    event->subrate_change.status,
                    event->subrate_change.subrate_factor);
        return 0;
#endif
#endif
    }
    return 0;
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
on_reset_cb(int reason) {
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

void gatt_svr_register_cb(struct ble_gatt_register_ctxt* ctxt, void* arg) {
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
