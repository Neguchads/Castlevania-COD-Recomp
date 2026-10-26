#include "../src/common/texture_lookup_rules.h"
#include <cstdio>
#include <string>

int main() {
    using namespace texture_lookup;
    // Nome do arquivo do pacote: 16 hex + .dds/.bmp (maiusculas ou minusculas); o resto fica fora do indice.
    auto stem = ParseTextureFile("00A1B2C3D4E5F607.dds");
    if (!stem || stem->hash != 0x00A1B2C3D4E5F607ull || !stem->dds) return 1;
    stem = ParseTextureFile("ffffffffffffffff.BMP");
    if (!stem || stem->hash != ~0ull || stem->dds) return 2;
    if (ParseTextureFile("00A1B2C3D4E5F60.dds") || ParseTextureFile("00A1B2C3D4E5F6070.dds") ||
        ParseTextureFile("00A1B2C3D4E5F60G.dds") || ParseTextureFile("00A1B2C3D4E5F607.png") ||
        ParseTextureFile("00A1B2C3D4E5F607.dds.bak") || ParseTextureFile("")) return 3;
    // Indice: primeira pasta vence (PT-BR antes dos packs); na mesma pasta DDS vence BMP.
    // caminhos longos (na heap): um move indevido deixaria a entrada vazia
    const std::string bmp = "mods/HD4K/textures/0000000000000005.bmp", dds = "mods/HD4K/textures/0000000000000005.dds";
    Index<std::string> index;
    index.Add(5, false, bmp);
    index.Add(5, true, dds);    // mesma pasta (rank 0): DDS troca o BMP
    index.NextFolder();
    index.Add(5, true, "mods/Outro/textures/0000000000000005.dds");  // pasta seguinte nao troca
    index.Add(9, false, "mods/Outro/textures/0000000000000009.bmp");
    if (!index.Find(5) || *index.Find(5) != dds || !index.Find(9) || index.Find(9)->empty() || index.Find(7)) return 4;
    Index<std::string> dds_first;  // DDS antes do BMP: o BMP nao troca
    dds_first.Add(5, true, dds);
    dds_first.Add(5, false, bmp);
    if (!dds_first.Find(5) || *dds_first.Find(5) != dds) return 9;
    // Hash por quadro: o mesmo quadro reaproveita, quadro novo calcula de novo.
    FrameMemo memo;
    int computed = 0;
    auto compute = [&] { computed++; return uint64_t(42 + computed); };
    if (memo.Get(1, 10, compute) != 43 || memo.Get(1, 10, compute) != 43 || computed != 1) return 5;
    if (memo.Get(2, 10, compute) != 44 || computed != 2) return 6;
    if (memo.Get(1, 11, compute) != 45 || computed != 3) return 7;
    // Memo nao cresce sem limite: chaves de quadros antigos saem.
    for (uint64_t k = 0; k < 10000; k++) memo.Get(100 + k, 12 + k / 1000, compute);
    if (memo.Size() > FrameMemo::kMaxEntries) return 8;
    std::puts("Texture lookup: file stems, folder priority, DDS over BMP, per-frame hash memo and bound passed");
}
