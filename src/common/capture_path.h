#pragma once
#include <filesystem>

namespace cod_capture {
inline std::filesystem::path Directory(const std::filesystem::path& configured) {
    // A captura manual funciona mesmo quando a variavel estava vazia no boot.
    return configured.empty() ? std::filesystem::path("captures/orange-marker") : configured;
}
}
