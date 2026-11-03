#include "../src/common/texture_pack.h"
#include <array>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>

int main(int argc,char** argv){
    std::array<uint8_t,58> bytes{};bytes[0]='B';bytes[1]='M';bytes[10]=54;bytes[14]=40;
    bytes[18]=1;bytes[22]=1;bytes[26]=1;bytes[28]=32;
    bytes[54]=0x11;bytes[55]=0x22;bytes[56]=0xCC;bytes[57]=0x80;
    auto image=dev::DecodePackBmp(bytes);
    if(!image || image->width!=1 || image->height!=1 || image->bgra!=std::vector<uint8_t>({0x11,0x22,0xCC,0x80})) return 1;
    if(dev::DecodePackBmp(std::span(bytes).first(55))) return 2;
    bytes[30]=1;if(dev::DecodePackBmp(bytes))return 3;bytes[30]=0;
    bytes[18]=0;if(dev::DecodePackBmp(bytes))return 4;bytes[18]=1;
    bytes[10]=1;if(dev::DecodePackBmp(bytes))return 5;bytes[10]=54;
    // Limite de lado: 16384 aceito (acima do antigo 4096), 16385 recusado.
    auto wide=[&](uint32_t width) {
        std::vector<uint8_t> row(54+size_t(width)*4);
        std::memcpy(row.data(),bytes.data(),54);std::memcpy(row.data()+18,&width,4);
        return dev::DecodePackBmp(row);
    };
    if(auto big=wide(dev::kMaxPackSide);!big || big->width!=16384 || big->height!=1) return 6;
    if(wide(dev::kMaxPackSide+1)) return 7;
    // DDS BC7 (como o texconv grava, com cabecalho DX10): niveis e tamanho dos blocos.
    auto dds=[](uint32_t width,uint32_t height,uint32_t levels,uint32_t format,uint32_t array,size_t extra) {
        std::vector<uint8_t> file(148);
        auto put=[&](size_t offset,uint32_t value){std::memcpy(file.data()+offset,&value,4);};
        std::memcpy(file.data(),"DDS ",4);put(4,124);put(12,height);put(16,width);put(28,levels);
        put(76,32);put(80,4);std::memcpy(file.data()+84,"DX10",4);
        put(128,format);put(132,3);put(140,array);
        size_t blocks=0;
        for(uint32_t l=0;l<std::max(levels,1u);++l)
            blocks+=size_t(std::max(1u,((width>>l)+3)/4))*std::max(1u,((height>>l)+3)/4)*16;
        file.resize(148+blocks+extra,0xAB);
        return file;
    };
    auto bc7=dev::DecodePackTexture(dds(8,8,4,98,1,0));
    if(!bc7 || bc7->format!=dev::PackFormat::Bc7 || bc7->width!=8 || bc7->height!=8 || bc7->levels!=4 ||
       bc7->bgra.size()!=size_t(64+16+16+16)) return 8;
    if(dev::PackLevelPitch(dev::PackFormat::Bc7,8)!=32 || dev::PackLevelSize(dev::PackFormat::Bc7,1,1)!=16 ||
       dev::PackLevelSize(dev::PackFormat::Bgra8,3,2)!=24) return 9;
    if(auto single=dev::DecodePackTexture(dds(4096,2048,0,98,1,0));!single || single->levels!=1) return 10;
    auto truncated=dds(8,8,4,98,1,0);truncated.pop_back();
    if(dev::DecodePackTexture(truncated)) return 11;
    if(dev::DecodePackTexture(dds(8,8,1,71,1,0)) || dev::DecodePackTexture(dds(8,8,1,98,2,0)) ||
       dev::DecodePackTexture(dds(8,8,5,98,1,0)) || dev::DecodePackTexture(dds(dev::kMaxPackSide*2,4,1,98,1,0))) return 12;
    if(auto bmp=dev::DecodePackTexture(bytes);!bmp || bmp->format!=dev::PackFormat::Bgra8 || bmp->levels!=1) return 13;
    // Opcional: pasta de um pacote real (ex.: mods/HD4K/textures) lida pelo mesmo decodificador do jogo.
    if(argc>1) {
        unsigned count=0;
        for(const auto& entry:std::filesystem::directory_iterator(argv[1])) {
            if(entry.path().extension()!=".dds" && entry.path().extension()!=".bmp") continue;
            std::ifstream stream(entry.path(),std::ios::binary);
            std::vector<uint8_t> file((std::istreambuf_iterator<char>(stream)),{});
            auto real=dev::DecodePackTexture(file);
            if(!real || !real->width || !real->height) { std::printf("invalido: %s\n",entry.path().filename().string().c_str()); return 14; }
            ++count;
        }
        if(!count) return 15;
        std::printf("Packs HD: %u texturas reais lidas pelo decodificador do jogo\n",count);
    }
    std::puts("Packs HD: BMP real, alpha, DDS BC7 com mipmaps e rejeicao de arquivo invalido passaram");
}
