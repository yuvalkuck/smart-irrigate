#include <iostream>
#include <CLI/CLI.hpp>
#include "extern.h"
int main(int argc, char **argv) {
    CLI::App app{"Generate client payloads"};
    bool isConfig;
    app.add_flag("-c", isConfig, "Total number of valves");
    CLI11_PARSE(app, argc, argv);
    if (isConfig) {
        auto tt = generate_config_payload(6,2,4);
        std::string str(reinterpret_cast<const char*>(tt.data()), tt.size());
        std::cout << str;
    }
}