#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <vector>

namespace dev {
enum class PackFormat : uint8_t { Bgra8, Bc7 };
// bgra: pixels BGRA8 (BMP) ou os blocos BC7 de todos os niveis em sequencia (DDS).
struct TextureImage {
    uint32_t width=0,height=0; std::vector<uint8_t> bgra;
    PackFormat format=PackFormat::Bgra8; uint32_t levels=1;
};
// Maior textura 2D do D3D12 (D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION).
inline constexpr uint32_t kMaxPackSide=16384;
// BC7: blocos de 4x4 pixels com 16 bytes; BGRA8: 4 bytes por pixel.
inline constexpr uint64_t PackLevelPitch(PackFormat format,uint32_t width) {
    return format==PackFormat::Bc7 ? uint64_t(std::max(1u,(width+3)/4))*16 : uint64_t(width)*4;
}
inline constexpr uint64_t PackLevelSize(PackFormat format,uint32_t width,uint32_t height) {
    return format==PackFormat::Bc7 ? PackLevelPitch(format,width)*std::max(1u,(height+3)/4) : uint64_t(width)*height*4;
}
inline std::optional<TextureImage> DecodePackBmp(std::span<const uint8_t> bytes) {
    if(bytes.size()<54 || bytes[0]!='B' || bytes[1]!='M') return std::nullopt;
    auto word=[&](size_t offset) {uint16_t value;std::memcpy(&value,bytes.data()+offset,2);return value;};
    auto dword=[&](size_t offset) {uint32_t value;std::memcpy(&value,bytes.data()+offset,4);return value;};
    uint32_t start=dword(10),header=dword(14),width=dword(18),raw_height=dword(22);
    int32_t height;std::memcpy(&height,&raw_height,4);
    uint32_t h=height<0 ? uint32_t(-int64_t(height)) : uint32_t(height);
    uint32_t bits=word(28);
    if(header<40 || start<54 || start<uint64_t(14)+header || !width || !h || width>kMaxPackSide || h>kMaxPackSide ||
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
// DDS BC7_UNORM com cabecalho DX10, como `texconv -f BC7_UNORM` grava; mipmaps opcionais.
inline std::optional<TextureImage> DecodePackDds(std::span<const uint8_t> bytes) {
    constexpr size_t kData=148;           // "DDS " + DDS_HEADER (124) + DDS_HEADER_DXT10 (20)
    constexpr uint32_t kBc7Unorm=98;      // DXGI_FORMAT_BC7_UNORM
    constexpr uint32_t kTexture2D=3;      // D3D10_RESOURCE_DIMENSION_TEXTURE2D
    if(bytes.size()<kData || std::memcmp(bytes.data(),"DDS ",4)!=0 || std::memcmp(bytes.data()+84,"DX10",4)!=0) return std::nullopt;
    auto dword=[&](size_t offset) {uint32_t value;std::memcpy(&value,bytes.data()+offset,4);return value;};
    const uint32_t height=dword(12),width=dword(16),levels=std::max(dword(28),1u);
    if(dword(4)!=124 || dword(128)!=kBc7Unorm || dword(132)!=kTexture2D || dword(140)!=1 ||
       !width || !height || width>kMaxPackSide || height>kMaxPackSide) return std::nullopt;
    uint32_t most=1;while((std::max(width,height)>>most)>0) ++most;
    if(levels>most) return std::nullopt;
    uint64_t total=0;
    for(uint32_t l=0;l<levels;++l) total+=PackLevelSize(PackFormat::Bc7,std::max(1u,width>>l),std::max(1u,height>>l));
    if(kData+total>bytes.size()) return std::nullopt;
    return TextureImage{width,height,std::vector<uint8_t>(bytes.begin()+kData,bytes.begin()+ptrdiff_t(kData+total)),
                        PackFormat::Bc7,levels};
}
inline std::optional<TextureImage> DecodePackTexture(std::span<const uint8_t> bytes) {
    if(bytes.size()>=4 && std::memcmp(bytes.data(),"DDS ",4)==0) return DecodePackDds(bytes);
    return DecodePackBmp(bytes);
}
}
