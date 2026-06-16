//
// Created by uv on 16/06/2026.
//

#ifndef PARENT_EXTERN_H
#define PARENT_EXTERN_H
#include <vector>
#include <cstdint>
std::vector<uint8_t> generate_config_payload(uint8_t totalValves, uint8_t programsPerValve, uint8_t tasksPerProgram);
#endif //PARENT_EXTERN_H