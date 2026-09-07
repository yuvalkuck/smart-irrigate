#include "irrigation.h"
#include "protocol.h"
#include "logger.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <ctime>
#include <optional>

static constexpr auto TAG = "Irrigation:";

static std::optional<ConfigurationContainer> s_config;
static const gpio_num_t *s_valvePins = nullptr;
static size_t s_valveCount = 0;
static esp_timer_handle_t s_timer = nullptr;
static ProgramType s_activeProgram = ProgramTyNormal;

// boundary is a minute-of-day [0,1440). Returns the number of minutes from
// now (today or, if it already passed, the same clock time tomorrow) until it occurs.
static int minutes_until(int boundary, int nowMinutes) {
    return boundary > nowMinutes ? boundary - nowMinutes : boundary - nowMinutes + 24 * 60;
}

// Interpretation of a Program's tasks is the same regardless of which
// Program type was selected as active -- selection only decides *which*
// program a valve consults, not how its tasks are read.
static void interpret_program_tasks(const Program *prog, int nowMinutes, bool &active, int &minMinutesUntilNext) {
    for (uint8_t t = 0; t < prog->noTasks; ++t) {
        const Task &task = prog->tasks[t];
        const int startMod = (task.hours * 60 + task.minutes) % (24 * 60);
        const int endMod = (task.hours * 60 + task.minutes + task.duration) % (24 * 60);
        const bool wraps = startMod > endMod;
        const bool taskActive = wraps
            ? (nowMinutes >= startMod || nowMinutes < endMod)
            : (nowMinutes >= startMod && nowMinutes < endMod);
        if (taskActive) {
            active = true;
        }
        for (int boundary : {startMod, endMod}) {
            const int until = minutes_until(boundary, nowMinutes);
            if (minMinutesUntilNext < 0 || until < minMinutesUntilNext) {
                minMinutesUntilNext = until;
            }
        }
    }
}

static void evaluate_and_reschedule() {
    if (!s_config) {
        return;
    }
    const auto *config = static_cast<const Configuration *>(s_config->get());

    const time_t now = time(nullptr);
    struct tm timeInfo{};
    localtime_r(&now, &timeInfo);
    if ((timeInfo.tm_year + 1900) < 2020) {
        ESP_LOGW(TAG, "System time not synchronized yet; deferring irrigation schedule");
        return;
    }
    const int nowMinutes = timeInfo.tm_hour * 60 + timeInfo.tm_min;

    int minMinutesUntilNext = -1;
    const auto *valvePtr = reinterpret_cast<const uint8_t *>(config->valves);

    for (uint8_t v = 0; v < config->noValves; ++v) {
        const auto *valve = reinterpret_cast<const Valve *>(valvePtr);
        const auto *progPtr = valvePtr + sizeof(Valve);
        bool active = false;

        for (uint8_t p = 0; p < valve->noPrograms; ++p) {
            const auto *prog = reinterpret_cast<const Program *>(progPtr);
            const bool isSelectedProgram = (prog->type == s_activeProgram);
            progPtr += sizeof(Program) + prog->noTasks * sizeof(Task);
            if (isSelectedProgram) {
                interpret_program_tasks(prog, nowMinutes, active, minMinutesUntilNext);
            }
        }

        if (valve->valveId >= 1 && static_cast<size_t>(valve->valveId) <= s_valveCount) {
            gpio_set_level(s_valvePins[valve->valveId - 1], active ? 1 : 0);
        } else {
            ESP_LOGE(TAG, "Configured valveId %u has no matching GPIO pin", valve->valveId);
        }

        valvePtr = progPtr;
    }

    if (minMinutesUntilNext >= 0) {
        esp_timer_stop(s_timer); // no-op if not currently running
        int64_t delayUs = (static_cast<int64_t>(minMinutesUntilNext) * 60 - timeInfo.tm_sec) * 1000000LL;
        if (delayUs < 1000000LL) {
            delayUs = 1000000LL;
        }
        esp_timer_start_once(s_timer, delayUs);
    }
}

static void cbIrrigationTimer(void *) {
    evaluate_and_reschedule();
}

void irrigation_recheck() {
    evaluate_and_reschedule();
}

void irrigation_set_active_program(ProgramType type) {
    s_activeProgram = type;
    evaluate_and_reschedule();
}

// Replaces s_config with a fresh mmap of the "config" partition. The old
// mapping (if any) is unmapped as a side effect of the std::optional/
// unique_ptr assignment below.
static bool reload_configuration_from_flash() {
    auto config = getConfiguration();
    if (!config) {
        ESP_LOGE(TAG, "No stored configuration found; valves will stay closed");
        return false;
    }
    const auto *header = static_cast<const Configuration *>(config->get());
    if (header->command.command != CommandConfiguration) {
        ESP_LOGE(TAG, "Stored configuration is not initialized; valves will stay closed");
        return false;
    }
    s_config = std::move(config);
    return true;
}

void irrigation_reload() {
    if (reload_configuration_from_flash()) {
        evaluate_and_reschedule();
    }
}

void start_irrigation(const gpio_num_t *valvePins, size_t valveCount) {
    METHODTRACE
    s_valvePins = valvePins;
    s_valveCount = valveCount;

    const esp_timer_create_args_t timerArgs = {
        .callback = &cbIrrigationTimer,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "irrigation",
    };
    ESP_ERROR_CHECK(esp_timer_create(&timerArgs, &s_timer));

    irrigation_reload();
}
