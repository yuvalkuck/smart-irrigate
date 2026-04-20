#include "hsntp.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_netif_sntp.h"
#include "sdkconfig.h"

static const char* TAG = "hSNTP:";
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void obtain_server_config(esp_sntp_config &config) {

    config.start = false;                       // start SNTP service explicitly (after connecting)
    config.server_from_dhcp = true;             // accept NTP offers from DHCP server, if any (need to enable *before* connecting)
    config.renew_servers_after_new_IP = true;   // let esp-netif update configured SNTP server(s) after receiving DHCP lease
    config.index_of_first_server = 1;           // updates from server num 1, leaving server 0 (from DHCP) intact
    config.ip_event_to_renew = IP_EVENT_ETH_GOT_IP;

}
static void print_servers(void)
{
    ESP_LOGI(TAG, "List of configured NTP servers:");

    for (uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i){
        if (esp_sntp_getservername(i)) {
            ESP_LOGI(TAG, "server %d: %s", i, esp_sntp_getservername(i));
            // } else {
            //     // we have either IPv4 or IPv6 address, let's print it
            //     char buff[INET6_ADDRSTRLEN];
            //     ip_addr_t const *ip = esp_sntp_getserver(i);
            //     if (ipaddr_ntoa_r(ip, buff, INET6_ADDRSTRLEN) != NULL)
            //         ESP_LOGI(TAG, "server %d: %s", i, buff);
            // }
        }
    }
}


esp_err_t init_sntp(const char *server) {
    ESP_LOGI(TAG, "INIT SNTP");
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(server);
    obtain_server_config(config);
    config.sync_cb = time_sync_notification_cb; // only if we need the notification function
    auto ret = esp_netif_sntp_init(&config);
    ESP_ERROR_CHECK(ret);
    return ret;
}
esp_err_t start_sntp(void) {
    ESP_LOGI(TAG, "START SNTP");
    auto ret = esp_netif_sntp_start();
    print_servers();
    ESP_ERROR_CHECK(ret);
    return ret;
}
