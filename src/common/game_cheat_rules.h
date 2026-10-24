#pragma once
#include <cstdint>

namespace dev {
inline bool BlockPlayerDamage(bool enabled, uint32_t stat, uint32_t slot, int32_t delta) {
    return enabled && stat == 6 && slot == 0 && delta < 0;
}
}
