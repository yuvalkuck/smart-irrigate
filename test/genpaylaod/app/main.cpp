#include <iostream>
#include <fstream>
#include "CLI/CLI.hpp"
#include "fmt/format.h"
#include "extern.h"

int main(int argc, char **argv) {
    CLI::App app{"Generate client payloads"};
    bool isConfig = false;
    std::string intoFile;
    uint totalValves = 6;
    uint8_t programsPerValve = 2;
    uint8_t tasksPerProgram = 4;
    app.add_flag("-c", isConfig, "generate binary config payload");
    app.add_option("-v", totalValves, fmt::format("Total number of valves (default:{})", totalValves));
    app.add_option("-p", programsPerValve, fmt::format("Total number of programs per valve (default:{})", programsPerValve));
    app.add_option("-t", tasksPerProgram, fmt::format("Total number of tasks per program (default:{})", tasksPerProgram));
    app.add_option("-o", intoFile, "write to file name");
    CLI11_PARSE(app, argc, argv);
    if (isConfig) {
        auto tt = generate_config_payload(totalValves,programsPerValve,tasksPerProgram);
        std::string str(reinterpret_cast<const char*>(tt.data()), tt.size());
        if (!intoFile.empty()) {
            std::ofstream out(intoFile, std::ios::binary);
            out << str;
        } else {
        std::cout << str;
    }
}
}