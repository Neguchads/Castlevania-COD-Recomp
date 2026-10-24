#pragma once
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <vector>

namespace dev {
struct TextureImage { uint32_t width=0,height=0; std::vector<uint8_t> bgra; };
inline std::optional<TextureImage> DecodePackBmp(std::span<const uint8_t> bytes) {
    if(bytes.size()<54 || bytes[0]!='B' || bytes[1]!='M') return std::nullopt;
    auto word=[&](size_t offset) {uint16_t value;std::memcpy(&value,bytes.data()+offset,2);return value;};
    auto dword=[&](size_t offset) {uint32_t value;std::memcpy(&value,bytes.data()+offset,4);return value;};
    uint32_t start=dword(10),header=dword(14),width=dword(18),raw_height=dword(22);
    int32_t height;std::memcpy(&height,&raw_height,4);
    uint32_t h=height<0 ? uint32_t(-int64_t(height)) : uint32_t(height);
    uint32_t bits=word(28);
    if(header<40 || start<54 || start<uint64_t(14)+header || !width || !h || width>4096 || h>4096 ||
       word(26)!=1 || (bits!=24 && bits!=32) || dword(30)!=0) return std::nullopt;
    uint64_t stride=(uint64_t(width)*(bits/8)+3)&~uint64_t(3);
    if(uint64_t(start)+stride*h>bytes.size()) return std::nullopt;
    TextureImage image{width,h,std::vector<uint8_t>(size_t(width)*h*4)};
    for(uint32_t y=0;y<h;y++) for(uint32_t x=0;x<width;x++) {
        size_t source=start+size_t(height<0?y:h-1-y)*size_t(stride)+x*(bits/8);
        size_t destination=(size_t(y)*width+x)*4;
        std::memcpy(image.bgra.data()+destination,bytes.data()+source,3);
        image.bgra[destination+3]=bits==32?bytes[source+3]:255;
    }
    return image;
}
}
