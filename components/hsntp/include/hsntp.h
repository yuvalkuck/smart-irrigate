#ifndef HSNTP_H
#define HSNTP_H

#include "esp_err.h"

typedef void (*timer_cb)(struct timeval *tv);

esp_err_t init_sntp(const char *server, timer_cb cb);
esp_err_t start_sntp(void);

#endif

