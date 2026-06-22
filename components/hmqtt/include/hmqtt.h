#ifndef HMQTT_H
#define HMQTT_H

#include "esp_err.h" // Include for esp_err_t

void init_hmqtt(void);
void start_hmqtt(void);
esp_err_t mqtt_publish(const char *topic, const char *payload);

#endif