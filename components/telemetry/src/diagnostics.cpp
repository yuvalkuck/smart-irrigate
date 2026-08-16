//
// Created by uv on 06/08/2026.
//
#include "diagnostics.h"
#include <type_traits>

namespace Diagnostics {
    // Hex values for targeting the I2C physical layer addresses
    constexpr MaskStateOK& operator|=(MaskStateOK& lhs, MaskStateOK rhs) {
        lhs = lhs | rhs;
        return lhs;
    }
    constexpr MaskStateOK operator|(MaskStateOK lhs, MaskStateOK rhs) {
        return static_cast<MaskStateOK>(
            static_cast<std::underlying_type_t<MaskStateOK>>(lhs) |
            static_cast<std::underlying_type_t<MaskStateOK>>(rhs)
        );
    }    


}