#if !defined(BASIC_PROTOCOL_H)
#define BASIC_PROTOCOL_H
#include "sdkconfig.h"
#if !defined(TESTER)
#include <esp_partition.h>
#include <memory>
#else
    #include <stdint.h>
#endif
#include <stdio.h>

extern "C" {
enum Command {
    CommandInvalid = 0,
    CommandConfiguration = 1,
    // CommandStartValveTask = 2,
    // CommandStopValve = 3,
    // CommandSuspendValve = 4,
    // CommandReleaseValve = 5,
    // CommandGetActiveStatus = 6,
    // CommandGetNextTask = 7,
    /* Segment 2 */
    CommandRestart = 11
};

typedef struct {
    uint32_t magic = CONFIG_PROTOCOL_MAGIC_NUMBER;
    uint16_t revision = 0x0001;
    uint16_t padding = 0x00;
} BaseProtocol;

typedef struct {
    const BaseProtocol magic;
    uint8_t client;
    uint8_t command;
} BaseCommand;

#if !defined(TESTER)
struct UnmapperPartitionDMA {
    esp_partition_mmap_handle_t handle;

    // The unique_ptr will call this operator automatically when destroyed
    void operator()(const void* ptr) const {
        if (ptr != nullptr && handle != 0) {
            esp_partition_munmap(handle);
        }
    }
};
#endif
}
#pragma pack(pop)
#endif
