#include <stdio.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "flash.h"
static const char* TAG = "flash:";
esp_err_t init_flash(void)
{
    ESP_LOGI(TAG, "nvs flash init");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}
