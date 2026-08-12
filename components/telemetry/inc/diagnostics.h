//
// Created by uv on 06/08/2026.
//

#ifndef IRRIGATE_DIAGNOSTICS_H
#define IRRIGATE_DIAGNOSTICS_H
#pragma once

#include "driver/i2c_master.h"
#include "esp_adc/adc_oneshot.h"
#include <type_traits>

// ====================================================================
// HARDWARE DIAGNOSTIC BITMASK POSITIONS (C++17 Binary Assignments)
// ====================================================================
namespace Diagnostics {
    enum class MaskStateOK : uint8_t {
        Reset = 0x00,
        SHT41 = (1 << 0),
        BMP581 = (1 << 1),
        TSL2591 = (1 << 2),
        DS18B20 = (1 << 3),
        XDB401 = (1 << 4),
        WindSpeed = (1 << 5),
        All = SHT41 | BMP581 | TSL2591 | DS18B20 | XDB401 | WindSpeed
    };

    constexpr MaskStateOK& operator|=(MaskStateOK& lhs, MaskStateOK rhs);
    constexpr MaskStateOK operator|(MaskStateOK lhs, MaskStateOK rhs);

    // Declaration only: Tells the compiler this function exists globally
    MaskStateOK execute_operational_self_test(i2c_master_bus_handle_t i2c_bus) noexcept;
}
#endif //IRRIGATE_DIAGNOSTICS_H
