#include "extern.h"
#include "configuration.h"
#include <vector>
#include <cstdint>
#include <ctime>
#include <random>
/**
 * Generates a contiguous flat binary buffer containing the configuration.
 * Uses C++20 chrono features and clean random distributions.
 */
std::vector<uint8_t> generate_config_payload(uint8_t totalValves, uint8_t programsPerValve, uint8_t tasksPerProgram) {
// 1. Calculate variable structure byte sizes
    const size_t taskLen = sizeof(Task);
    const size_t programLen = sizeof(Program) + (tasksPerProgram * taskLen);
    const size_t valveLen = sizeof(Valve) + (programsPerValve * programLen);
    const size_t totalSize = sizeof(Configuration) + (totalValves * valveLen);

    // 2. Allocate the contiguous block memory buffer
    std::vector<uint8_t> buffer(totalSize, 0);
    uint8_t* ptr = buffer.data();

    // 3. Setup C++17 Timestamp & Time Structure
    std::time_t rawTime = std::time(nullptr);
    std::tm* timeInfo = std::localtime(&rawTime); // Use std::gmtime(&rawTime) if UTC is preferred

    // 4. Setup random engines (Supported since C++11/14/17)
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> startDist(0, 239);    // Random start 0 to 4 hours in minutes
    std::uniform_int_distribution<> durationDist(15, 150); // Random duration up to 150 minutes
    std::uniform_int_distribution<> gapDist(5, 90);       // Random arbitrary gaps up to 90 minutes

    // 5. Populate Header Configuration (Using standard pointer casting for C++17)
    Configuration* config = reinterpret_cast<Configuration*>(ptr);
    config->timestamp = static_cast<uint32_t>(rawTime);
    config->todayIndex = static_cast<uint8_t>(timeInfo->tm_wday); // Returns 0 (Sun) through 6 (Sat)
    config->noValves = totalValves;
    config->padding = 0;

    ptr += sizeof(Configuration);

    // 6. Populate Nested Data Layout
    for (uint8_t v = 0; v < totalValves; ++v) {
        Valve* valve = reinterpret_cast<Valve*>(ptr);
        valve->valveId = v + 1;
        valve->noPrograms = programsPerValve;

        uint8_t* progPtr = ptr + sizeof(Valve);

        for (uint8_t p = 0; p < programsPerValve; ++p) {
            Program* prog = reinterpret_cast<Program*>(progPtr);
            prog->noTasks = tasksPerProgram;
            prog->type = p;
            prog->padding = 0;

            // Random initial start offset for this sequence block
            int currentTotalMinutes = startDist(gen);

            for (uint8_t t = 0; t < tasksPerProgram; ++t) {
                // Deconstruct time safely rolling over midnight bounds using math loops
                prog->tasks[t].hours = static_cast<uint8_t>((currentTotalMinutes / 60) % 24);
                prog->tasks[t].minutes = static_cast<uint8_t>(currentTotalMinutes % 60);

                // Assign dynamic randomized duration
                prog->tasks[t].duration = static_cast<uint16_t>(durationDist(gen));

                // Progress absolute timeline pointer past processing duration AND dynamic random gaps
                currentTotalMinutes += prog->tasks[t].duration + gapDist(gen);
            }

            progPtr += programLen;
        }

        ptr += valveLen;
    }

    return buffer;
}
