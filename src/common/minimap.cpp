#include "dev_runtime.h"
#include "guest_memory_guard.h"
#include "minimap_model.h"
#include "minimap_state.h"
#include "kernel/kernel.h"
#include "core/log.h"
#include <imgui.h>
#include <map>
#include <limits>

namespace dev {
namespace {
using cod_map::Point;
using cod_map::Triangle;
struct Room { int id=0,x=0,y=0,rotation=0;uint32_t resource=0;std::vector<Triangle> triangles;std::vector<std::pair<Point,Point>> edges;uint32_t fragments=0;int8_t fragment_count=0; };
std::map<uint32_t,std::vector<Triangle>> models;
int cached_stage=-1;
std::vector<Room> rooms;
bool loaded=false;
template<class T> bool Guest(uint32_t address,T& value) {
    if(!ReadableGuestRange(kernel::g_base,address,sizeof(T))) return false;
    std::memcpy(&value,kernel::g_base+address,sizeof(T));return true;
}
void LoadModels() {
    if(loaded) return;
    loaded=true;
    for(const char* name:{"chra","clka","gara","kepa","kepb","kepc","mora","muga","roma","twna","wala","yama"}) {
        std::vector<uint8_t> bytes;
        if(!kernel::ReadGameFile(std::string("D:\\data\\stmap_")+name+".dar",bytes)) continue;
        uint32_t magic=0,count=0;
        if(!cod_map::Read(bytes,0,magic) || magic!=0x82734927 || !cod_map::Read(bytes,16,count) || count>4096) continue;
        for(uint32_t i=0;i<count;++i) {
            uint32_t id=0,type=0,offset=0,size=0;const size_t entry=32+size_t(i)*16;
            if(!cod_map::Read(bytes,entry,id) || !cod_map::Read(bytes,entry+4,type) || type ||
               !cod_map::Read(bytes,entry+8,offset) || !cod_map::Read(bytes,entry+12,size) ||
               uint64_t(entry)+offset+size>bytes.size()) continue;
            auto triangles=cod_map::Model(std::span<const uint8_t>(bytes).subspan(entry+offset,size));
            if(!triangles.empty()) models.emplace(id,std::move(triangles));
        }
    }
    LOGR("minimap: native room models loaded=%u",unsigned(models.size()));
}
bool LoadRooms(int stage) {
    constexpr uint32_t header=0x55BA00;
    uint32_t flags=0,groups=0;int16_t count=0;
    if(!Guest(header+4,flags) || !Guest(header+10,count) || count<1 || count>32 || !Guest(header+12,groups)) return false;
    auto pointer=[&](uint32_t value){return flags&1 ? value : header+value;};
    groups=pointer(groups);
    for(int i=0;i<count;++i) {
        int8_t id=0,n=0;uint32_t records=0;
        if(!Guest(groups+i*8,id) || !Guest(groups+i*8+1,n) || id!=stage || n<1 ||
           !Guest(groups+i*8+4,records)) continue;
        records=pointer(records);rooms.clear();
        for(int j=0;j<n;++j) {
            const uint32_t record=records+j*24;int16_t x=0,y=0,r=0;uint8_t room=0;uint32_t resource=0;
            if(!Guest(record,x) || !Guest(record+2,y) || !Guest(record+4,r) || r<0 || r>3 ||
               !Guest(record+8,room) || !Guest(record+12,resource)) continue;
            Room entry{int(room),x,y,r,resource,{},{}};
            uint32_t fragments=0;
            if(Guest(record+9,entry.fragment_count) && Guest(record+16,fragments) && fragments)
                entry.fragments=pointer(fragments);
            if(auto found=models.find(resource);found!=models.end()) for(const auto& triangle:found->second) {
                auto project=[&](Point p){return *cod_map::NativePoint(p,x,y,r,true);};
                entry.triangles.push_back({project(triangle.a),project(triangle.b),project(triangle.c)});
            }
            using Key=std::array<int,4>;
            std::map<Key,std::pair<int,std::pair<Point,Point>>> edges;
            auto edge=[&](Point a,Point b) {
                std::array<int,2> ka{int(std::round(a.x*32)),int(std::round(a.y*32))},kb{int(std::round(b.x*32)),int(std::round(b.y*32))};
                if(kb<ka){std::swap(ka,kb);std::swap(a,b);}
                auto& e=edges[{ka[0],ka[1],kb[0],kb[1]}];++e.first;e.second={a,b};
            };
            for(const auto& t:entry.triangles){edge(t.a,t.b);edge(t.b,t.c);edge(t.c,t.a);}
            for(const auto& [key,e]:edges) if(e.first==1) entry.edges.push_back(e.second);
            if(!entry.triangles.empty()) rooms.push_back(std::move(entry));
        }
        cached_stage=stage;return !rooms.empty();
    }
    return false;
}
bool Player(Point& world,int& stage,int& room) {
    uint32_t player=0;int16_t index=0,state=0;uint8_t flags=0;int8_t s=0,r=0;float x=0,z=0;
    if(!Guest(0xB41394,player) || !player || player>0xFFFFFF00u ||
       !Guest(player+0x144,index) || index<0 || index>127 ||
       !Guest(player+0x146+uint32_t(index)*2,state) || state!=2 ||
       !Guest(player+0xEE,flags) || (flags&0x18)!=0x18 ||
       !Guest(player+0xD0,x) || !Guest(player+0xD8,z) || !std::isfinite(x) || !std::isfinite(z) ||
       !Guest(0xB234F4,s) || !Guest(0xB234F5,r) || s<2 || s>13 || r<0) return false;
    world={x,z};stage=s;room=r;return true;
}
}
void DrawMinimap() {
    if(!minimap_enabled || !kernel::g_base) return;
    Point world;int stage=0,room_id=0;
    if(!Player(world,stage,room_id)) return;
    LoadModels();
    if(stage!=cached_stage && !LoadRooms(stage)) return;
    const Room* current=nullptr;
    for(const auto& room:rooms) if(room.id==room_id){current=&room;break;}
    if(!current) return;
    auto marker=cod_map::NativePoint(world,current->x,current->y,current->rotation);
    if(!marker) return;
    // Janela local centrada no jogador: conserva tamanho das salas entre areas.
    const auto& io=ImGui::GetIO();
    const float ui_scale=std::clamp(std::min(io.DisplaySize.x/1920,io.DisplaySize.y/1080),.65f,2.0f)*minimap_scale.load();
    const ImVec2 size{260*ui_scale,150*ui_scale},origin{io.DisplaySize.x-size.x-22*ui_scale,22*ui_scale};
    const float zoom=.32f*ui_scale;
    const float heading=minimap_rotating ? camera_heading.load()-current->rotation*1.5707963268f : 0;
    auto screen=[&](Point p){
        const auto delta=cod_map::HeadingUp({p.x-marker->x,p.y-marker->y},heading);
        return ImVec2{origin.x+size.x*.5f+delta.x*zoom,origin.y+size.y*.5f-delta.y*zoom};
    };
    const float alpha=minimap_opacity.load();
    auto color=[&](int r,int g,int b,float a){return IM_COL32(r,g,b,int(255*alpha*a));};
    auto* draw=ImGui::GetBackgroundDrawList();
    draw->PushClipRect(origin,{origin.x+size.x,origin.y+size.y},true);
    for(const auto& room:rooms) {
        const bool active=room.id==room_id;
        const auto tint=cod_map::Tint(cod_map::NativeRoomState(stage,room.id,room.fragments,room.fragment_count,[](uint32_t address,auto& value){return Guest(address,value);}),active);
        for(const auto& triangle:room.triangles) {
            const auto a=screen(triangle.a),b=screen(triangle.b),c=screen(triangle.c);
            draw->AddTriangleFilled(a,b,c,color(tint.r,tint.g,tint.b,tint.fill));
        }
        for(const auto& edge:room.edges)
            draw->AddLine(screen(edge.first),screen(edge.second),color(tint.r,tint.g,tint.b,tint.edge),ui_scale);
    }
    const ImVec2 center{origin.x+size.x*.5f,origin.y+size.y*.5f};
    draw->AddCircleFilled(center,7*ui_scale,color(255,195,89,.2f));
    draw->AddCircleFilled(center,3*ui_scale,color(255,218,140,1));
    const auto direction=cod_map::HeadingUp({std::sin(camera_heading.load()-current->rotation*1.5707963268f),
        std::cos(camera_heading.load()-current->rotation*1.5707963268f)},heading);
    const ImVec2 tip{center.x+direction.x*10*ui_scale,center.y-direction.y*10*ui_scale};
    draw->AddLine(center,tip,color(255,228,170,1),2*ui_scale);
    draw->PopClipRect();
    draw->AddRect(origin,{origin.x+size.x,origin.y+size.y},color(181,169,146,.6f),2*ui_scale,0,ui_scale);
}
}
