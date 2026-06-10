#include "command.h"
extern "C" {
    // - Every x days,weeks, minutes
    //   - 01:0123456 - days - char bitwise
    //   - 02:48 - max weeks (1-52)
    // - Every x minutes (x = 5,10,15,30,60)
    //   - 03:1440 - max minutes (1-120)
    //   - 04: reserved
    enum RepeatType {
        RepeatInvalid = 0,
        RepeatDays = 1,
        RepeatWeeks = 2,
        RepeatMinutes = 3,
        RepeatReserved = 4
    };
    typedef struct {
        uint16_t type;
        uint16_t value;
    } Repeat;
    typedef struct  {
        Command command;
        Repeat repeat;
        uint16_t todayIndex; // reset when get to maximum
        uint32_t timestamp;
    } Configuraion;
}