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
    CommandRestart = 11
};
```
The content of Command enum can change depend on BaseCommand revision.
# Configuration
configuration is based on BaseCommand structure and contains the following (all structures are
`#pragma pack(push, 4)`-packed, matching `components/protocol/include/configuration.h`):

```C++
typedef enum ProgramType_t {
    ProgramTyNormal = 0x01,
    ProgramTyEmergency = 0x02
} ProgramType;

typedef struct {
    uint8_t hours; // 0-23
    uint8_t minutes; // 0-59
    uint16_t duration; // minutes
} Task;

typedef struct {
    uint8_t noTasks;
    uint8_t type; // ProgramTyNormal or ProgramTyEmergency
    uint16_t padding;
    Task tasks[];
} Program;

// program len = sizeof(Program) + noTasks * sizeof(Task)

typedef struct {
    uint8_t noPrograms;
    uint8_t valveId;
    Program programs[];
} Valve;

// valve len = sizeof(Valve) + noPrograms * program len (each program's own len, they need not match)

typedef struct {
    BaseCommand command;
    uint32_t timestamp;
    uint8_t todayIndex;
    uint8_t noValves;
    uint16_t padding = 0;
    Valve valves[];
} Configuration;
```

There is no explicit `len` field on `Program`/`Valve`/`Configuration` — every nested structure's byte
length is derived from its own count field (`noTasks`, `noPrograms`) rather than being stored
inline, so parsing must walk the flat buffer sequentially rather than jumping by a fixed stride.
Device firmware only acts on `Program`s of type `ProgramTyNormal` today; `ProgramTyEmergency`
programs are parsed but not yet actuated (switching to emergency mode, whether by direct user action
or an external AI decision, is not implemented).



