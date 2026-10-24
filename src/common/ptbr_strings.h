#pragma once
#include <map>
#include <cstdint>

#if __has_include("../../assets/ptbr_strings.private.h")
#include "../../assets/ptbr_strings.private.h"
#else
namespace cod_ptbr {
inline const std::map<uint32_t,const char*> translations{};
}
#endif
