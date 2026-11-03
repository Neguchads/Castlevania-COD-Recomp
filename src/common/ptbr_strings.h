#pragma once
#include <map>
#include <cstdint>

// Build de desenvolvimento: o catalogo privado entra compilado. Build publico (COD_PUBLIC_BUILD no CMake):
// nunca entra, e o texto vem do pacote mods/PTBR-PS2/ptbr_strings.bin (ptbr_catalog.h).
#if !defined(COD_NO_PRIVATE_CATALOG) && __has_include("../../assets/ptbr_strings.private.h")
#include "../../assets/ptbr_strings.private.h"
#else
namespace cod_ptbr {
inline const std::map<uint32_t,const char*> translations{};
}
#endif
