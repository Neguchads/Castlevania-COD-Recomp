#include "../src/common/texture_pack.h"
#include <array>
#include <cstdio>

int main(){
    std::array<uint8_t,58> bytes{};bytes[0]='B';bytes[1]='M';bytes[10]=54;bytes[14]=40;
    bytes[18]=1;bytes[22]=1;bytes[26]=1;bytes[28]=32;
    bytes[54]=0x11;bytes[55]=0x22;bytes[56]=0xCC;bytes[57]=0x80;
    auto image=dev::DecodePackBmp(bytes);
    if(!image || image->width!=1 || image->height!=1 || image->bgra!=std::vector<uint8_t>({0x11,0x22,0xCC,0x80})) return 1;
    if(dev::DecodePackBmp(std::span(bytes).first(55))) return 2;
    bytes[30]=1;if(dev::DecodePackBmp(bytes))return 3;bytes[30]=0;
    bytes[18]=0;if(dev::DecodePackBmp(bytes))return 4;bytes[18]=1;
    bytes[10]=1;if(dev::DecodePackBmp(bytes))return 5;
    std::puts("Packs HD: BMP real, alpha e rejeicao de arquivo invalido passaram");
}
