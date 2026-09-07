#ifndef IRRIGATION_H
#define IRRIGATION_H
#include "driver/gpio.h"
#include "configuration.h"
#include <cstddef>

// Loads the stored Configuration from the "config" partition and starts
// driving valvePins according to the active program's schedule
// (ProgramTyNormal, until something switches to ProgramTyEmergency).
void start_irrigation(const gpio_num_t *valvePins, size_t valveCount);

// Re-evaluates the schedule against the current time. Must be called once
// the system clock is known-good (e.g. after SNTP sync), since the device
// has no battery-backed RTC and boots at epoch time.
void irrigation_recheck();

// Re-reads the Configuration from the "config" partition (discarding the
// previous mmap) and re-evaluates the schedule against it. Call after a new
// configuration has been written to flash, e.g. once setConfiguration()
// succeeds for a COMMON_EVENT_ACCEPT_SERVER_CONFIGURATION event.
void irrigation_reload();

// Switches which Program type is scheduled and immediately re-evaluates
// valve state under it. This is the extension point for a future emergency
// trigger -- a direct user action or an AI decision on the companion S3 --
// neither of which is wired up to call it yet.
void irrigation_set_active_program(ProgramType type);

#endif
