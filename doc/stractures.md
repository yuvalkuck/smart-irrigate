# Introduction
## Communication
done by MQTT server that this app is it client

## MQTT
command structure is binary, ad will use the following format:

```C++
typedef struct {
    uint16_t revision = 0x01;
    uint8_t client;         // client id
    uint8_t command;        // command id
} BaseCommand;
```
The structure of BaseCommand but it will effect the revision id.

command are part of enum Command:
```C++
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
    CommandRestart = 16
};
```
The content of Command enum can change depend on BaseCommand revision.
# Configuration
configuration is based on BaseCommand structure and contains the following:

```C++
typedef struct {
    uint16_t startTime;
    uint16_t duration;
} Task;

typedef struct {
    uint16_t len; // length in bytes of Program
    uint16_t type;
    Task tasks[];
} Program;

typedef struct {
    uint16_t len;
    uint8_t noPrograms;
    uint8_t valveId;
    Program programs[];
} Valve;

typedef struct {
    BaseCommand command;
    uint32_t timestamp;
    uint8_t todayIndex;
    uint8_t noValves;
    uint16_t len;
    Valve valves[];
} Configuraion;
```



