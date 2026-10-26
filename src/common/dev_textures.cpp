#include "dev_runtime.h"
#include "core/log.h"
#include <array>
#include <climits>
#include <cstring>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

namespace dev {
std::shared_ptr<const TextureImage> ReplacementTexture(uint64_t hash) {
    // Este cache pertence exclusivamente a thread de renderizacao.
    static std::unordered_map<uint64_t,std::shared_ptr<const TextureImage>> cache;
    static unsigned generation=UINT_MAX;
    if(generation!=texture_generation.load()) {cache.clear();generation=texture_generation.load();texture_replacements.store(0);}
    if(auto found=cache.find(hash);found!=cache.end()) return found->second;
    if(cache.size()>4096) cache.clear();
    std::shared_ptr<const TextureImage> image;
    // falta nao entra no cache: o indice responde sem disco, e faltas (texturas dinamicas mudam de hash a
    // cada quadro) enchiam o cache ate o limpar inteiro, relendo do disco cada textura HD em uso
    const auto path=ModTexture(hash);
    if(!path) return image;
    std::error_code ec;
    auto length=std::filesystem::file_size(*path,ec);
    if(!ec && length>=54 && length<=70*1024*1024) {
        std::ifstream input(*path,std::ios::binary);
        std::vector<uint8_t> bytes(size_t(length),uint8_t(0));
        if(input.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size()))) {
            if(auto decoded=DecodePackTexture(bytes)) {image=std::make_shared<TextureImage>(std::move(*decoded));texture_replacements.fetch_add(1);}
            else LOGE("mods: invalid texture %s (BMP 24/32 bits or DDS BC7_UNORM)",path->filename().string().c_str());
        }
    }
    // arquivo invalido tambem fica: sem reler do disco a cada desenho
    cache.emplace(hash,image);return image;
}
void DumpTexture(uint64_t hash,uint32_t width,uint32_t height,const std::vector<uint8_t>& pixels) {
    static std::unordered_set<uint64_t> done;
    if(!texture_dump || done.size()>=200 || pixels.size()!=size_t(width)*height*4 || !done.insert(hash).second) return;
    auto directory=Root()/"texture-dumps";
    std::error_code ec;std::filesystem::create_directories(directory,ec);
    if(ec) return;
    char name[32];std::snprintf(name,sizeof(name),"%016llX.bmp",static_cast<unsigned long long>(hash));
    std::ofstream output(directory/name,std::ios::binary);
    std::array<uint8_t,54> header{};header[0]='B';header[1]='M';
    auto put=[&](size_t offset,uint32_t value){std::memcpy(header.data()+offset,&value,4);};
    put(2,uint32_t(54+pixels.size()));put(10,54);put(14,40);put(18,width);put(22,uint32_t(-int32_t(height)));
    header[26]=1;header[28]=32;put(34,uint32_t(pixels.size()));
    output.write(reinterpret_cast<const char*>(header.data()),header.size());
    output.write(reinterpret_cast<const char*>(pixels.data()),std::streamsize(pixels.size()));
    if(output) texture_exports.fetch_add(1);
}
}
