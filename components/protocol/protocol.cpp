#include "protocol.h"

#include <cstring>

#include "esp_partition.h"
#include "esp_heap_caps.h"
#include "esp_log.h"

static const char* TAG = "ExProtocol:";
#define PARTITION_CONFIG_NAME "config"
std::optional<ConfigurationContainer> getConfiguration() {
    const esp_partition_t *partition = esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA,
            ESP_PARTITION_SUBTYPE_DATA_UNDEFINED,
            PARTITION_CONFIG_NAME
        );

    if (partition == nullptr) {
        ESP_LOGE(TAG, "Partition '%s' not found.", PARTITION_CONFIG_NAME);
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
static std::vector<uint8_t> init_buff;
static std::vector<uint8_t> *createEmptyConfiguration() {
    Configuration empty{};
    ESP_LOGI(TAG, "Create empty configuration");
    empty.command.command = CommandConfiguration;
    init_buff.assign((uint8_t*)&empty, (uint8_t*)&empty + sizeof(empty));
    return &init_buff;
}
Command getPayloadCommand(std::vector<uint8_t> *payload) {
    auto *cmd = reinterpret_cast<BaseCommand*>(payload->data());
    return static_cast<Command>(cmd->command);
}

bool setConfiguration(const std::vector<uint8_t> *buff = createEmptyConfiguration()) {
    const esp_partition_t* partition = esp_partition_find_first(
            ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, PARTITION_CONFIG_NAME
        );

    if (partition == nullptr) {
        ESP_LOGE(TAG, "Store failed: Partition '%s' not found.", PARTITION_CONFIG_NAME);
        return false;
    }

    if (buff->size() > partition->size) {
        ESP_LOGE(TAG, "Store failed: Data size exceeds partition size. %i > %i", buff->size(),partition->size);
        return false;
    }

    // Flash memory must be erased before writing. 
    // Sector size on ESP32 is always 4096 bytes. Align erase size up to 4KB.
    size_t erase_size = (buff->size() + 4095) & ~4095;
    
    ESP_LOGI(TAG, "Erasing %d bytes...", erase_size);
    esp_err_t err = esp_partition_erase_range(partition, 0, erase_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erase failed: %s", esp_err_to_name(err));
        return false;
    }

    // Write the raw data to the physical offset 0 of the partition
    ESP_LOGI(TAG, "Writing data to flash...");
    err = esp_partition_write(partition, 0, buff, buff->size());
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Write failed: %s", esp_err_to_name(err));
        return false;
    }

    return true;
}


