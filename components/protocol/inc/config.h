#include "command.h"
#pragma pack(push, 4)
extern "C" {
/*
    |- config
        |--- valve
            |--- valveFullLength
            |--- noPrograms
            |--- program
                |--- noTasks
                |--- task - fixed size
            |--- program
                |--- task
*/
#define ProgramTyNormal 0x01
#define ProgramTyEmergency = 0x02
typedef struct {
    uint8_t hours;      // 0-23
    uint8_t minutes;    // 0-59
    uint16_t duration;
} Task;

typedef struct {
    uint8_t noTasks;
    uint8_t type;
    uint16_t padding;
    Task tasks[];
} Program;
    // program len = sizeof(Program) + noTasks * sizeof(Task)

typedef struct {
    uint8_t noPrograms;
    uint8_t valveId;
    Program programs[];
} Valve;
    // valve len = sizeof(Valve) + noPrograms * program len

typedef struct {
    BaseCommand command;
    uint32_t timestamp;
    uint8_t todayIndex;
    uint8_t noValves;
    uint16_t len;
    Valve valves[];
} Configuraion;
}
#pragma pack(pop)