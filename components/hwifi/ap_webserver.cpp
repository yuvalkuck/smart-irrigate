//
// Created by uv on 12/07/2026.
//
#include "wifi_softap.h"
#include "flash.h"
#include <array>
#include <esp_http_server.h>
#include <logger.h>
#include <fmt/core.h>

#include "default_initiate.h"
#include "mod_types_gen.h"

static const char* TAG = "WebServer";
#include <esp_http_server.h>
static void register_routes(httpd_handle_t server);

httpd_handle_t start_webserver() {
    METHODTRACE
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Memory optimizations for ESP-IDF 6.x
    config.stack_size = 4096; // Down from 4096 (minimum safe stack)
    config.max_open_sockets = 2; // Down from 7 (limits concurrent clients)
    config.max_uri_handlers = 4; // Allocate only what you need
    config.recv_wait_timeout = 2; // Low timeout frees sockets faster
    config.send_wait_timeout = 2;

    httpd_handle_t server = nullptr;
    if (httpd_start(&server, &config) == ESP_OK) {
        register_routes(server);
        // Register URI handlers here
        return server;
    }
    return nullptr;
}


// 1. Define the GET Handler Function
static esp_err_t get_rootpage(httpd_req_t* req) {
    METHODTRACE
    return ESP_OK;
}
static esp_err_t get_version_handler(httpd_req_t* req) {
    METHODTRACE
    char payload[1024] = {0};
    fmt::format_to_n(payload, sizeof(payload), R"({{"{}"="{}","{}"="{}"}})",
                         "device", CONST_PROJECT_VERSION,
                         "esp-idf", esp_get_idf_version());
    httpd_resp_send(req, payload, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t get_command_restart(httpd_req_t* req) {
    METHODTRACE
    esp_restart();
    return ESP_OK;
}


#define MAX_STRING_LENGTH 128

#define SEND_BUF_LEN 192

// The reply (protocol/schema/mod_types.schema.json) is streamed in chunks from the generated
// table, so no buffer holds the whole document. An empty chunk ends the response, never send one.
static bool send_raw(httpd_req_t* req, const char* str) {
    return httpd_resp_send_chunk(req, str, HTTPD_RESP_USE_STRLEN) == ESP_OK;
}

template<typename... Args>
static bool send_fmt(httpd_req_t* req, fmt::format_string<Args...> format, Args&&... args) {
    char buf[SEND_BUF_LEN];
    const auto res = fmt::format_to_n(buf, sizeof(buf), format, std::forward<Args>(args)...);
    const size_t len = res.size < sizeof(buf) ? res.size : sizeof(buf);
    return httpd_resp_send_chunk(req, buf, len) == ESP_OK;
}

#define SEND_RAW(req, str) do { if (!send_raw(req, str)) return false; } while (0)
#define SEND(req, ...) do { if (!send_fmt(req, __VA_ARGS__)) return false; } while (0)

// Sends a JSON string body (no surrounding quotes) escaped.
static bool send_escaped(httpd_req_t* req, const char* str) {
    char buf[64];
    size_t len = 0;
    for (; *str; ++str) {
        if (len + 6 >= sizeof(buf)) {
            if (httpd_resp_send_chunk(req, buf, len) != ESP_OK) return false;
            len = 0;
        }
        const unsigned char ch = *str;
        if (ch == '"' || ch == '\\') {
            buf[len++] = '\\';
            buf[len++] = ch;
        } else if (ch < 0x20) {
            len += snprintf(buf + len, 7, "\\u%04x", ch);
        } else {
            buf[len++] = ch;
        }
    }
    return len == 0 || httpd_resp_send_chunk(req, buf, len) == ESP_OK;
}

static bool read_mask(const NvsConfig& cfg, const char* key, uint8_t bits, uint32_t& out) {
    if (bits <= 16) {
        uint16_t value = 0;
        if (!cfg.getMask(key, value)) return false;
        out = value;
        return true;
    }
    return cfg.getMask(key, out);
}

static bool send_section(httpd_req_t* req, const NvsConfig& cfg, const mod_types::Section& sec, char* str) {
    SEND(req, R"(,"{}":{{"title":"{}","return":"{}")", sec.name, sec.title, sec.mask_mode ? "mask" : "key");
    uint32_t mask = 0;
    bool haveMask = false;
    if (sec.mask_mode) {
        SEND(req, R"(,"key":"{}","bits":{})", sec.key, static_cast<unsigned>(sec.bits));
        haveMask = read_mask(cfg, sec.key, sec.bits, mask);
    }
    SEND_RAW(req, R"(,"properties":{)");
    for (size_t i = 0; i < sec.count; ++i) {
        const mod_types::Property& prop = sec.props[i];
        SEND(req, R"({}"{}":{{"title":"{}")", i ? "," : "", prop.name, prop.title);
        if (prop.key) {
            SEND(req, R"(,"key":"{}")", prop.key);
        }
        if (prop.type == mod_types::Type::String) {
            SEND(req, R"(,"default":"{}")", prop.def);
            if (prop.has_range) {
                SEND(req, R"(,"range":[{},{}])", prop.range_min, prop.range_max);
            }
        } else {
            SEND(req, R"(,"default":{})", prop.def);
            if (sec.mask_mode) {
                SEND(req, R"(,"shift":{})", static_cast<unsigned>(prop.shift));
            }
        }
        if (prop.mask) {
            SEND_RAW(req, R"(,"mask":true)");
        }
        // a missing NVS key has no "value": the iSetup shows the default
        if (prop.type == mod_types::Type::String) {
            if (prop.key && cfg.getStr(prop.key, str, MAX_STRING_LENGTH)) {
                SEND_RAW(req, R"(,"value":")");
                if (!send_escaped(req, str)) return false;
                SEND_RAW(req, "\"");
            }
        } else if (haveMask) {
            SEND(req, R"(,"value":{})", (mask >> prop.shift) & 1 ? "true" : "false");
        }
        SEND_RAW(req, "}");
    }
    SEND_RAW(req, "}}");
    return true;
}

static bool send_status(httpd_req_t* req) {
    NvsConfig hCfg;
    char str[MAX_STRING_LENGTH];
    SEND_RAW(req, R"({"format":{"ver":)");
    SEND(req, "{}", mod_types::kFormatVer);
    SEND_RAW(req, "}");
    for (const mod_types::Section& sec : mod_types::sections) {
        if (!send_section(req, hCfg, sec, str)) return false;
    }
    SEND_RAW(req, "}");
    return true;
}

static esp_err_t get_status_handler(httpd_req_t* req) {
    METHODTRACE
    httpd_resp_set_type(req, "application/json");
    if (!send_status(req)) {
        return ESP_FAIL;
    }
    return httpd_resp_send_chunk(req, nullptr, 0);
}

// Decodes little-endian hex ("4404" -> 0x0444, 4 or 8 digits) into out; returns the byte count, 0 on error.
static size_t decode_hex_le(const char* hex, uint32_t& out) {
    const size_t len = strlen(hex);
    if (len != 4 && len != 8) return 0;
    out = 0;
    for (size_t i = 0; i < len / 2; ++i) {
        int nibble[2];
        for (int n = 0; n < 2; ++n) {
            const char ch = hex[2 * i + n];
            nibble[n] = ch >= '0' && ch <= '9' ? ch - '0'
                      : ch >= 'a' && ch <= 'f' ? ch - 'a' + 10
                      : ch >= 'A' && ch <= 'F' ? ch - 'A' + 10 : -1;
            if (nibble[n] < 0) return 0;
        }
        out |= static_cast<uint32_t>(nibble[0] << 4 | nibble[1]) << (8 * i);
    }
    return len / 2;
}

// How the values of an ini section are stored, chosen by the section header.
enum class SectionKind { String, Binary, Number };

// 2. Define the POST Handler Function
// Body is an ini-style payload of "key=value" lines. The section header sets how the following
// values are stored (section names are lower case): "[binary]" is little-endian hex of a stored
// mask, "[number]" is accepted but not supported yet, so its lines are ignored with a warning, any
// other header (or none) is a string, e.g.:
//   [string]
//   wifi_ssid=MySSID
//   wifi_password=secret
//   [binary]
//   enable_hardw=44040000
static esp_err_t post_handler(httpd_req_t* req) {
    METHODTRACE
    int remaining = req->content_len;
    ESP_LOGI(TAG, "content_len: %d", remaining);
    if (remaining <= 0 || remaining >= 1024) {
        return ESP_FAIL;
    }

    char body[1024];
    int received = 0;
    while (received < remaining) {
        int ret = httpd_req_recv(req, body + received, remaining - received);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) continue;
            return ESP_FAIL;
        }
        received += ret;
    }
    body[received] = 0;

    NvsConfig hCfg;
    char* saveptr = nullptr;
    SectionKind kind = SectionKind::String;
    char* line = strtok_r(body, "\r\n", &saveptr);
    while (line) {
        if (line[0] == '[') {
            kind = strcmp(line, "[binary]") == 0 ? SectionKind::Binary
                 : strcmp(line, "[number]") == 0 ? SectionKind::Number
                 : SectionKind::String;
        } else {
            auto value = strchr(line, '=');
        if (!value) {
                ESP_LOGW(TAG, "no '=' in line: %s", line);
            } else {
        *value = 0; value++;
                ESP_LOGI(TAG, "accept: %s=%s", line, value);
                if (kind == SectionKind::String) {
                    // the read side (get_status_handler) holds MAX_STRING_LENGTH bytes including the NUL
                    if (strlen(value) >= MAX_STRING_LENGTH) {
                        ESP_LOGW(TAG, "cut %s to %d characters", line, MAX_STRING_LENGTH - 1);
                        value[MAX_STRING_LENGTH - 1] = 0;
                    }
                    hCfg.setStr(line, value);
                } else if (kind == SectionKind::Binary) {
                    uint32_t mask;
                    const size_t bytes = decode_hex_le(value, mask);
                    // set = keep only the bits of the value (subMask ANDs), then add them (addMask ORs)
                    if (bytes == 2) {
                        hCfg.subMask(line, static_cast<uint16_t>(mask));
                        hCfg.addMask(line, static_cast<uint16_t>(mask));
                    } else if (bytes == 4) {
                        hCfg.subMask(line, mask);
                        hCfg.addMask(line, mask);
                    } else {
                        ESP_LOGW(TAG, "bad binary value for %s", line);
                    }
                } else {
                    // SectionKind::Number: add the number support here
                    ESP_LOGW(TAG, "number sections are not supported yet, ignored: %s", line);
                }
            }
        }
        line = strtok_r(nullptr, "\r\n", &saveptr);
    }

    httpd_resp_send_chunk(req, NULL, 0); // End response
    return ESP_OK;
}


// 3. Registering the Handlers inside your server initialization
static void register_routes(httpd_handle_t server) {
    METHODTRACE
    // GET Route Configuration
    {
        httpd_uri_t get_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = get_rootpage,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &get_uri);
    }
    {
        httpd_uri_t get_uri = {
            .uri = "/api",
            .method = HTTP_GET,
            .handler = get_version_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &get_uri);
    }

    {
        httpd_uri_t get_uri = {
            .uri = "/api/restart",
            .method = HTTP_POST,
            .handler = get_command_restart,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &get_uri);
    }

    httpd_uri_t get_uri = {
        .uri = "/api/status",
        .method = HTTP_GET,
        .handler = get_status_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &get_uri);

    // POST Route Configuration
    httpd_uri_t post_uri = {
        .uri = "/api/set",
        .method = HTTP_POST,
        .handler = post_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &post_uri);
}
