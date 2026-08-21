#include <vector>
#include <span>
#include "logger.h"
#include "mqtt_client.h"
#include "hmqtt.h"
#include "flash.h"
#include "common_event.h"

static const char* TAG = "hmqtt:";
extern const uint8_t ca_crt_start[] asm("_binary_ca_crt_start");
extern const uint8_t client_crt_start[] asm("_binary_client_crt_start");
extern const uint8_t client_key_start[] asm("_binary_client_key_start");

static void log_error_if_nonzero(const char* message, int error_code) {
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

// Declare static buffers to store MQTT configuration strings
static char mqtt_broker_uri[256] = {0};
static char mqtt_username[64] = {0};
static char mqtt_password[128] = {0};

static esp_mqtt_client_config_t mqtt_cfg;
static esp_mqtt_client_handle_t hClient;

// Define the absolute maximum binary payload size you expect to receive
constexpr size_t MAX_EXPECTED_PAYLOAD_SIZE = 4096;

// Persistent buffer to avoid constantly allocating/freeing memory on the heap
std::vector<uint8_t> reassembly_buffer;
static size_t total_expected_data = 0;
// topic is only valid on the first chunk of a message, so it must be saved here
// to be checked once reassembly completes on a later chunk
static char reassembly_topic[64] = {0};

#define TOPIC_CONFIGURATION "/client/configuration"

static void mqttEventHandler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    auto event = esp_mqtt_event_handle_t(static_cast<esp_mqtt_event_handle_t>(event_data));
    int msg_id;
    switch (static_cast<esp_mqtt_event_id_t>(event_id)) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(hClient, TOPIC_CONFIGURATION, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        {
            // Rule: Only the FIRST chunk of a message contains a valid topic pointer and event->total_data_len
            if (event->current_data_offset == 0) {
                total_expected_data = event->total_data_len;
                reassembly_buffer.clear();

                size_t topic_len = std::min<size_t>(event->topic_len, sizeof(reassembly_topic) - 1);
                memcpy(reassembly_topic, event->topic, topic_len);
                reassembly_topic[topic_len] = '\0';

                // Safety Check: Avoid out-of-memory crashes if a message is too large
                if (total_expected_data > MAX_EXPECTED_PAYLOAD_SIZE) {
                    ESP_LOGE(TAG, "Incoming message too large (%d bytes). Upper limit is %d.",
                             total_expected_data, MAX_EXPECTED_PAYLOAD_SIZE);
                    total_expected_data = 0; // Abort processing this message
                    return;
                }

                // Pre-allocate space at once to prevent sequential reallocations
                reassembly_buffer.reserve(total_expected_data);

                ESP_LOGI(TAG, "New incoming message. Total size: %d bytes", total_expected_data);
            }

            // If we aborted due to size error, drop subsequent chunks
            if (total_expected_data == 0) return;

            // Append the current chunk into our persistent buffer
            const auto* chunk_ptr = reinterpret_cast<const uint8_t*>(event->data);
            reassembly_buffer.insert(reassembly_buffer.end(), chunk_ptr, chunk_ptr + event->data_len);

            // Check if we have received the complete message
            if (reassembly_buffer.size() >= total_expected_data) {
                total_expected_data = 0;
                ESP_LOGI(TAG, "Message fully reassembled successfully!");

                // Pass a safe, zero-copy span window of the complete buffer to your parser
                if (strcmp(reassembly_topic, TOPIC_CONFIGURATION) == 0) {
                    ESP_LOGI(TAG, "Received configuration: %i", reassembly_buffer.size());
                    EventData event = {reassembly_buffer.size(), reassembly_buffer.data()};
                    esp_event_post(COMMON_BASE_EVENTS, COMMON_EVENT_ACCEPT_SERVER_CONFIGURATION, &event, event.length(), portMAX_DELAY);
                }
                // Clear tracking variables for the next incoming MQTT message

            }
        }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
                log_error_if_nonzero("captured as transport's socket errno",
                                     event->error_handle->esp_transport_sock_errno);
                ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

void init_hmqtt() {
    METHODTRACE
    NvsConfig hCfg;

    if (hCfg.getStr(CFG_NVS_KEY_MQTT_URL, mqtt_broker_uri, sizeof(mqtt_broker_uri))) {
        mqtt_cfg.broker.address.uri = mqtt_broker_uri;
    } else {
        ESP_LOGE(TAG, "Failed to get MQTT URL");
        return;
    }

    if (hCfg.getStr(CFG_NVS_KEY_MQTT_USERNAME, mqtt_username, sizeof(mqtt_username))) {
        mqtt_cfg.credentials.username = mqtt_username;
    } else {
        ESP_LOGE(TAG, "Failed to get MQTT Username");
        // It might be acceptable to proceed without a username if the broker allows it,
        // but for now, we'll return.
        return;
    }

    if (hCfg.getStr(CFG_NVS_KEY_MQTT_PASSWORD, mqtt_password, sizeof(mqtt_password))) {
        mqtt_cfg.credentials.authentication.password = mqtt_password;
    } else {
        ESP_LOGE(TAG, "Failed to get MQTT Password");
        // Similar to username, decide if this is a critical failure.
        return;
    }

    mqtt_cfg.broker.verification.certificate = (const char*)ca_crt_start;
    mqtt_cfg.credentials.authentication.certificate = (const char*)client_crt_start;
    mqtt_cfg.credentials.authentication.key = (const char*)client_key_start;
    mqtt_cfg.broker.verification.skip_cert_common_name_check = true;

    hClient = esp_mqtt_client_init(&mqtt_cfg);
    if (hClient == nullptr) {
        ESP_LOGE(TAG, "esp_mqtt_client_init failed");
    } else {
        /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
        esp_mqtt_client_register_event(hClient, MQTT_EVENT_ANY, mqttEventHandler, NULL);
    }
}

void start_hmqtt() {
    METHODTRACE
    esp_mqtt_client_start(hClient);
}

esp_err_t mqtt_publish(const char* topic, const char* payload) {
    METHODTRACE
    ESP_LOGI(TAG, "%s:[%s]->[%s]", __func__, topic, payload);
    return esp_mqtt_client_publish(hClient, topic, payload, strlen(payload), 1, 0);
}