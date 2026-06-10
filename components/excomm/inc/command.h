#if !defined(EXCOMM_COMMAND_H)
#define EXCOMM_COMMAND_H

extern "C" {
enum Command {
    CommandInvalid = 0,
    CommandConfiguration = 1,
    CommandStartValveTask = 2,
    CommandStopValve = 3,
    CommandSuspendValve = 4,
    CommandReleaseValve = 5,
    CommandGetActiveStatus = 6,
    CommandGetNextTask = 7,
    /* Segment 2 */
    CommandRestart = 11
};

typedef struct {
    uint16_t revision = 0x01;
    uint16_t command;
} BaseCommand;
}
#endif
