#if !defined(PROTOCOL_H)
#define PROTOCOL_H
#include "configuration.h"
#include <optional>
#include <vector>
std::optional<ConfigurationContainer> getConfiguration();
bool setConfiguration(const char *, uint16_t);
Command getPayloadCommand(std::vector<uint8_t> *);

#endif
