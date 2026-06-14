#include "command.h"

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
}
