#pragma once
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>

namespace cod_graphics {
inline std::optional<uint32_t> IndexedVertexBytes(std::span<const uint8_t> indices, uint32_t stride, uint32_t capacity) {
    if (indices.empty() || indices.size() % 2 || !stride) return std::nullopt;
    uint32_t maximum = 0;
    for (size_t at = 0; at < indices.size(); at += 2) {
        uint16_t index;
        std::memcpy(&index, indices.data() + at, sizeof(index));
        if (index > maximum) maximum = index;
    }
    const uint64_t bytes = (uint64_t(maximum) + 1) * stride;
    if (bytes > capacity) return std::nullopt;
    return uint32_t(bytes);
}
}
