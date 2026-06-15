#include "protocol.h"
#include "esp_partition.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include <cstddef>
static const char* TAG = "ExProtocol:";
#define CONFIG_PARTITION_NAME "config"
std::optional<ConfigurationContainer> getConfiguration() {
    const esp_partition_t *partition = esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA,
            ESP_PARTITION_SUBTYPE_DATA_UNDEFINED,
            CONFIG_PARTITION_NAME
        );

    if (partition == nullptr) {
        ESP_LOGE(TAG, "Partition '%s' not found.", CONFIG_PARTITION_NAME);
        return std::nullopt;
    }

    const void* mapped_ptr = nullptr;
    esp_partition_mmap_handle_t map_handle = 0;

    // Map the entire partition size into the chip's MMU
    esp_err_t err = esp_partition_mmap(
        partition,
        0,
        partition->size,
        ESP_PARTITION_MMAP_DATA,
        &mapped_ptr,
        &map_handle
    );

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Mapping failed with error: %s", esp_err_to_name(err));
        return std::nullopt;
    }

    // --- ZERO HEAP ACCESS ---
    // You can now read from s_mapped_ptr as if it were a local array
    return ConfigurationContainer(mapped_ptr, UnmapperPartitionDMA{map_handle});
}


