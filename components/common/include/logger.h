//
// Created by uv on 08/08/2026.
//

#ifndef SMART_IRRIGATE_LOGGER_H
#define SMART_IRRIGATE_LOGGER_H

#include <cstdint>
#include <cstring>
#include "esp_log.h"

class MethodTracer {
    char name_[64]{};

public:
    MethodTracer(const char* name, const char* file, int line, uint8_t len) {
        std::memmove(&name_, name, len > sizeof(name_) ? sizeof(name_) : len);
        ESP_LOGI(name_, " >>> Enter : %s (%i)", file, line);
    }

    ~MethodTracer() {
        ESP_LOGI(name_, " <<< Exit");
    }
};
#ifdef  _DEBUG
#define METHODTRACE MethodTracer(__FUNCTION__,__FILE_NAME__,__LINE__,sizeof(__FUNCTION__)-1);
#define LOGTRACE(tag, ...) ESP_LOGI(tag, ##__VA_ARGS__);
#else
#define METHODTRACE
#define LOGTRACE(fmt, ...)
#endif

#endif //SMART_IRRIGATE_LOGGER_H
