#ifndef HMQTT_H
#define HMQTT_H

void init_hmqtt(void);
void start_hmqtt(void);
int mqtt_publish(const char *topic, const char *payload);

#endif