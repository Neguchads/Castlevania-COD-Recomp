#include "../src/common/minimap_math.h"
#include "../src/common/minimap_model.h"
#include "../src/common/minimap_state.h"
#include <map>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <limits>
int main() {
    using namespace cod_map;
    std::map<uint32_t,uint32_t> state{{0xB24640,0x1000},{0x100C,0x2000},{0x2020,0x3000},
        {0x303C,0x490},{0x5000,9},{0x548CC8,0x4000},{0x4009,1}};
    auto read=[&](uint32_t address,auto& value){auto p=state.find(address);if(p==state.end())return false;value=static_cast<decltype(+value)>(p->second);return true;};
    const auto save=NativeRoomState(2,3,0x5000,1,read);
    if(!save.explored || save.type!=0x490 || Tint(save,false).r!=175)return 14;
    state[0x4009]=2;
    const auto dark=NativeRoomState(2,3,0x5000,1,read);
    if(dark.explored || Tint(dark,false).fill>=Tint(save,false).fill || Tint(dark,true).r!=175)return 15;
    state.erase(0xB24640);
    if(NativeRoomState(2,3,0x5000,1,read).explored || NativeRoomState(-1,3,0x5000,1,read).explored)return 16;
    const auto north=HeadingUp({0,1},0),east=HeadingUp({1,0},1.5707963268f);
    if(north.y!=1 || std::abs(east.x)>.001f || std::abs(east.y-1)>.001f) return 13;
    const auto view=Fit({-100,-50,100,50},{260,140},10);
    if(!view || view->scale!=1.2f) return 1;
    const auto center=view->Apply({0,0}),corner=view->Apply({-100,-50});
    if(center.x!=130 || center.y!=70 || std::abs(corner.x-10)>.001f || std::abs(corner.y-10)>.001f) return 2;
    const auto square=Fit({0,0,100,100},{260,140},10);
    if(!square || square->scale!=1.2f || std::abs(square->Apply({0,0}).x-70)>.001f) return 3;
    if(Fit({0,0,0,10},{260,140},10) || Fit({0,0,10,10},{20,20},10)) return 4;
    if(Fit({0,0,std::numeric_limits<float>::quiet_NaN(),1},{260,140},10)) return 5;
    const Point expected[]{{-24,64},{-16,-280},{-360,-288},{-48,56}};
    for(int i=0;i<4;++i) {
        const auto point=NativePoint({100,200},2,3,i);
        if(!point || std::abs(point->x-expected[i].x)>.001f || std::abs(point->y-expected[i].y)>.001f) return 6;
    }
    if(NativePoint({0,0},0,0,4) || NativePoint({std::numeric_limits<float>::infinity(),0},0,0,0)) return 7;
    unsigned models=0,triangles=0;
    for(const auto& file:std::filesystem::directory_iterator("assets/data")) {
        if(!file.path().filename().string().starts_with("stmap_")) continue;
        std::ifstream input(file.path(),std::ios::binary);
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
        uint32_t count=0;if(!Read(bytes,16,count) || count>4096) return 8;
        for(uint32_t i=0;i<count;++i) {
            uint32_t id=0,kind=0,offset=0,length=0;
            if(!Read(bytes,32+i*16,id) || !Read(bytes,36+i*16,kind) ||
               !Read(bytes,40+i*16,offset) || !Read(bytes,44+i*16,length) || uint64_t(32+i*16)+offset+length>bytes.size()) return 9;
            if(kind || id<0x1510 || id>0x1640) continue;
            auto mesh=Model(std::span<const uint8_t>(bytes).subspan(32+i*16+offset,length));
            if(mesh.empty()){std::printf("Model rejected: %s resource %X\n",file.path().string().c_str(),id);return 12;}
            ++models;triangles+=unsigned(mesh.size());
        }
    }
    if(models!=240 || triangles!=39316) return 10;
    const std::vector<uint8_t> invalid(80);
    if(!Model(invalid).empty()) return 11;
    std::printf("Native minimap models=%u triangles=%u\n",models,triangles);
    std::puts("Minimap: aspect ratio, centering, negative coordinates and invalid input passed");
}
