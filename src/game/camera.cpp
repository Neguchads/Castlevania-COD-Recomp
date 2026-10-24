#include "xdk/hle.h"
#include "common/dev_runtime.h"
#include "common/guest_memory_guard.h"
#include "common/presentation_math.h"
#include "common/guest_test_memory.h"
#include "core/log.h"
#include <array>
#include <chrono>
#include <cstring>
#include <mutex>

X86_EXTERN_FUNC(__imp__sub_00362190);
X86_EXTERN_FUNC(__imp__sub_003623B0);
X86_EXTERN_FUNC(__imp__sub_003623F0);
X86_EXTERN_FUNC(__imp__sub_002FE650);
X86_EXTERN_FUNC(__imp__sub_00334640);
namespace {
using cod_view::Vec3;
template<class T> T Read(const uint8_t* base,uint32_t address) {T value{};std::memcpy(&value,base+address,sizeof(T));return value;}
template<class T> void Write(uint8_t* base,uint32_t address,T value) {std::memcpy(base+address,&value,sizeof(T));}
bool Current(uint8_t* base,uint32_t view) {
    if(!dev::ReadableGuestRange(base,view,0x178) || !dev::ReadableGuestRange(base,0xC36B74,4) ||
       Read<uint32_t>(base,0xC36B74)!=view || !dev::ReadableGuestRange(base,0xB41394,4)) return false;
    const auto player=Read<uint32_t>(base,0xB41394);
    if(!player || !dev::ReadableGuestRange(base,player,0x248)) return false;
    const auto index=Read<int16_t>(base,player+0x144);
    if(index<0 || index>127 || Read<int16_t>(base,player+0x146+index*2)!=2) return false;
    const auto descriptor=Read<uint32_t>(base,0xC36B70);
    if(!descriptor || !dev::ReadableGuestRange(base,descriptor+0x1D0,4)) return false;
    const auto callback=Read<uint32_t>(base,descriptor+0x1D0);
    return callback!=0x345020 && callback!=0x4E7D0;
}
bool ViewArg(X86Context& ctx,uint8_t* base,uint32_t& view) {
    if(!dev::ReadableGuestRange(base,ctx.esp,8)) return false;
    view=Read<uint32_t>(base,ctx.esp+4);return Current(base,view);
}
struct FlyInput {float forward=0,side=0,vertical=0,yaw=0,pitch=0;int mouse_x=0,mouse_y=0;};
std::mutex fly_lock;
FlyInput fly_input;
bool flying=false;
Vec3 fly_position;float fly_yaw=0,fly_pitch=0;
bool orbiting=false;
Vec3 orbit_target;
float orbit_yaw=0,orbit_pitch=.25f;
float orbit_manual_delay=0;
uint32_t orbit_view=0;
uint16_t orbit_area=0;
std::chrono::steady_clock::time_point fly_time;
bool Finite(Vec3 value) {return std::isfinite(value.x)&&std::isfinite(value.y)&&std::isfinite(value.z);}
bool PlayerPose(uint8_t* base,Vec3& position,float& yaw) {
    if(!dev::ReadableGuestRange(base,0xB41394,4))return false;
    const auto handle=Read<uint32_t>(base,0xB41394);
    if(!handle || !dev::ReadableGuestRange(base,handle,4))return false;
    const auto index=Read<uint32_t>(base,handle);
    if(index>=128)return false;
    const auto record=0xB4129Cu+index*0x1210u;
    if(!dev::ReadableGuestRange(base,record+0x10,0x20))return false;
    // Mesmos campos dos getters nativos 334640 (posicao) e 334510 (yaw).
    position=Read<Vec3>(base,record+0x10);yaw=Read<float>(base,record+0x2C);
    return Finite(position)&&std::isfinite(yaw);
}
}
namespace dev {
void OrbitCameraInput(input::Pad& pad,int mx,int my) {
    auto axis=[](int16_t value){return std::abs(int(value))>8000 ? value/32767.0f : 0.0f;};
    std::lock_guard guard(fly_lock);
    fly_input.yaw=axis(pad.rx);fly_input.pitch=-axis(pad.ry);
    fly_input.mouse_x+=mx;fly_input.mouse_y+=my;
    // O jogo ja interpreta LS no referencial da view corrente; RS pertence ao free look.
    pad.rx=0;pad.ry=0;
}
void FreeCameraInput(input::Pad& pad,bool forward,bool back,bool left,bool right,bool up,bool down,int mx,int my) {
    auto axis=[](int16_t value){return std::abs(int(value))>8000 ? value/32767.0f : 0.0f;};
    std::lock_guard guard(fly_lock);
    fly_input.forward=std::clamp(axis(pad.ly)+float(forward)-float(back),-1.0f,1.0f);
    fly_input.side=std::clamp(axis(pad.lx)+float(right)-float(left),-1.0f,1.0f);
    fly_input.vertical=std::clamp(float(up)-float(down)+(pad.analog[7]-pad.analog[6])/255.0f,-1.0f,1.0f);
    fly_input.yaw=axis(pad.rx);fly_input.pitch=-axis(pad.ry);
    fly_input.mouse_x+=mx;fly_input.mouse_y+=my;
    pad={};pad.connected=true;
}
}
X86_FUNC(sub_00362190) {
    uint32_t view=0;const int mode=dev::camera_mode.load();
    if(mode<1 || mode>3 || !ViewArg(ctx,base,view)) {__imp__sub_00362190(ctx,base);return;}
    const auto saved=Read<Vec3>(base,view+0x10);
    if(!Finite(saved)){__imp__sub_00362190(ctx,base);return;}
    float distance=dev::camera_distance.load(),height=dev::camera_height.load();
    if(mode==1){distance=1.4f;height=110;}
    if(mode==2){distance=.8f;height=35;}
    Write(base,view+0x10,Vec3{saved.x*distance,saved.y*distance+height,saved.z*distance});
    __imp__sub_00362190(ctx,base);Write(base,view+0x10,saved);
}
X86_FUNC(sub_003623B0) {
    uint32_t view=0;
    const int requested=dev::camera_mode.load();
    const bool orbit=requested==5;
    Vec3 player_position{};float player_yaw=0;
    const bool pose_valid=!orbit || PlayerPose(base,player_position,player_yaw);
    const auto scene=Read<uint32_t>(base,0x12B6FAC);
    const bool scene_valid=scene && dev::ReadableGuestRange(base,scene,16);
    const bool active=(requested==4 || (orbit && scene_valid && pose_valid)) && ViewArg(ctx,base,view);
    dev::camera_live.store(active);
    if(!active) {
        const bool valid_view=ViewArg(ctx,base,view);
        flying=false;orbiting=false;__imp__sub_003623B0(ctx,base);
        if(valid_view) {
            const auto camera=Read<Vec3>(base,view+0x40),target=Read<Vec3>(base,view+0x30);
            if(Finite(camera)&&Finite(target)) dev::camera_heading.store(std::atan2(target.x-camera.x,target.z-camera.z));
        }
        return;
    }
    std::array<uint8_t,0x90> saved{};std::memcpy(saved.data(),base+view,saved.size());
    const auto callback=Read<uint32_t>(base,view+0x150);
    const auto mode=Read<uint16_t>(base,view+0x170);
    const uint16_t area=Read<uint16_t>(base,0xB414E8);
    const Vec3 wanted=cod_view::OrbitTarget(player_position,dev::camera_height.load());
    const bool relocated=orbit && (view!=orbit_view || area!=orbit_area ||
        std::hypot(wanted.x-orbit_target.x,wanted.z-orbit_target.z)>2500);
    if(!flying || orbit!=orbiting || relocated) {
        fly_position=Read<Vec3>(base,view+0x40);
        const auto target=Read<Vec3>(base,view+0x30);
        if(!Finite(fly_position)||!Finite(target)){__imp__sub_003623B0(ctx,base);return;}
        const Vec3 delta{target.x-fly_position.x,target.y-fly_position.y,target.z-fly_position.z};
        fly_yaw=std::atan2(delta.x,delta.z);fly_pitch=std::atan2(delta.y,std::hypot(delta.x,delta.z));
        orbit_yaw=player_yaw;orbit_pitch=.25f;orbit_target=wanted;orbit_manual_delay=0;
        orbit_view=view;orbit_area=area;
        flying=true;orbiting=orbit;fly_time=std::chrono::steady_clock::now();
    }
    const auto now=std::chrono::steady_clock::now();
    const float dt=std::clamp(std::chrono::duration<float>(now-fly_time).count(),0.0f,.05f);fly_time=now;
    FlyInput input;
    {std::lock_guard guard(fly_lock);input=fly_input;fly_input.mouse_x=fly_input.mouse_y=0;}
    if(!dev::GameFocused() || dev::UiCapturing()) input={};
    fly_yaw+=input.yaw*dt*1.8f+input.mouse_x*.003f;
    dev::camera_heading.store(fly_yaw);
    fly_pitch=std::clamp(fly_pitch+input.pitch*dt*1.8f-input.mouse_y*.003f,-1.45f,1.45f);
    const auto direction=cod_view::Direction(fly_yaw,fly_pitch);
    const float speed=dev::camera_fly_speed.load()*dt;
    fly_position.x+=(direction.x*input.forward+std::cos(fly_yaw)*input.side)*speed;
    fly_position.y+=(direction.y*input.forward+input.vertical)*speed;
    fly_position.z+=(direction.z*input.forward-std::sin(fly_yaw)*input.side)*speed;
    Vec3 target{fly_position.x+direction.x*100,fly_position.y+direction.y*100,fly_position.z+direction.z*100};
    Vec3 position=fly_position;
    if(orbit) {
        const float smoothing=1-std::exp(-dev::orbit_smoothing.load()*dt);
        {
            orbit_target.x+=(wanted.x-orbit_target.x)*smoothing;
            orbit_target.y+=(wanted.y-orbit_target.y)*smoothing;
            orbit_target.z+=(wanted.z-orbit_target.z)*smoothing;
        }
        const bool manual=input.yaw!=0 || input.pitch!=0 || input.mouse_x!=0 || input.mouse_y!=0;
        orbit_manual_delay=manual ? 1.5f : std::max(0.f,orbit_manual_delay-dt);
        if(orbit_manual_delay==0)orbit_yaw=cod_view::FollowYaw(orbit_yaw,player_yaw,smoothing);
        orbit_yaw+=input.yaw*dt*1.8f+input.mouse_x*.003f;
        orbit_pitch=std::clamp(orbit_pitch+input.pitch*dt*1.8f+input.mouse_y*.003f,-.8f,1.3f);
        const float distance=400*dev::camera_distance.load();
        target=orbit_target;
        position=cod_view::OrbitPosition(target,orbit_yaw,orbit_pitch,distance);
        // Consulta original da camera com margem nativa de duas unidades.
        const uint32_t stack=ctx.esp;
        if(stack>=256 && dev::ReadableGuestRange(base,stack-256,256)) {
            const uint32_t scratch=stack-192;
            Write(base,scratch,std::array<float,4>{target.x,target.y,target.z,1});
            Write(base,scratch+16,std::array<float,4>{position.x,position.y,position.z,1});
            Write(base,scratch+32,std::array<float,4>{});
            X86Context query=ctx;query.esp=stack-256;
            Write(base,query.esp,uint32_t(0));Write(base,query.esp+4,scratch+32);
            Write(base,query.esp+8,scratch);Write(base,query.esp+12,scratch+16);
            __imp__sub_002FE650(query,base);
            const auto correction=Read<Vec3>(base,scratch+32);
            if(Finite(correction)){position.x+=correction.x;position.y+=correction.y;position.z+=correction.z;}
        }
        dev::camera_heading.store(orbit_yaw);
    }
    Write(base,view+0x30,target);
    Write(base,view+0x40,position);
    std::memset(base+view+0x50,0,32);
    Write(base,view+0x170,uint16_t(1));Write(base,view+0x150,uint32_t(0x3622C0));
    __imp__sub_003623B0(ctx,base);
    const auto look_direction=Read<std::array<float,4>>(base,view+0x70);
    std::memcpy(base+view,saved.data(),saved.size());Write(base,view+0x150,callback);Write(base,view+0x170,mode);
    // Direcao derivada deve acompanhar a matriz orbital para os consumidores de movimento.
    if(orbit)Write(base,view+0x70,look_direction);
}
X86_FUNC(sub_003623F0) {
    uint32_t view=0;const int mode=dev::camera_mode.load();
    if(!mode || !ViewArg(ctx,base,view)){__imp__sub_003623F0(ctx,base);return;}
    const auto focal=Read<int32_t>(base,view+0x154);
    const float field=mode==1 ? .95f : mode==2 ? 1.1f : dev::camera_field.load();
    Write(base,view+0x154,cod_view::Focal(focal,field));
    __imp__sub_003623F0(ctx,base);Write(base,view+0x154,focal);
}
namespace dev {
bool VerifyCameraHooks() {
    GuestTestMemory arena;if(!arena.data() || !kernel::g_base) return false;
    auto* memory=arena.data();
    std::memcpy(memory+0x420000,kernel::g_base+0x420000,0x160000);
    constexpr uint32_t view=0x220000,player=0x230000,descriptor=0x240000;
    Write(memory,0xC36B74,view);Write(memory,0xC36B70,descriptor);Write(memory,0xB41394,player);
    Write(memory,player,uint32_t(3));
    const uint32_t record=0xB4129C+3*0x1210;
    Write(memory,record+0x10,Vec3{100,500,200});Write(memory,record+0x2C,1.25f);
    Write(memory,player+0xD0,Vec3{-999,-999,-999});
    Vec3 pose{};float yaw=0;
    const bool pose_read=PlayerPose(memory,pose,yaw);
    X86Context getter{};getter.esp=GuestTestMemory::stack;
    Write(memory,getter.esp+4,player);Write(memory,getter.esp+8,uint32_t(0x280000));
    __imp__sub_00334640(getter,memory);
    const auto native_position=Read<Vec3>(memory,0x280000);
    const bool pose_ok=pose_read&&pose.x==native_position.x&&pose.y==native_position.y&&pose.z==native_position.z&&
        pose.x==100&&pose.y==500&&pose.z==200&&yaw==1.25f&&cod_view::OrbitTarget(pose,30).y==350;
    Write(memory,descriptor+0x1D0,uint32_t(0x2FE150));
    Write(memory,player+0x144,int16_t(0));Write(memory,player+0x146,int16_t(2));
    Write(memory,view+0x10,Vec3{100,20,300});Write(memory,view+0x40,Vec3{0,0,300});
    std::memcpy(memory+view+0x90,kernel::g_base+0x447628,64);
    Write(memory,view+0x158,int32_t(1024));
    const int previous=camera_mode.load();const float distance=camera_distance.load(),height=camera_height.load();
    auto apply=[&](int mode) {
        camera_mode=mode;X86Context call{};call.esp=GuestTestMemory::stack;Write(memory,call.esp+4,view);
        sub_00362190(call,memory);return Read<Vec3>(memory,view+0xC0);
    };
    const auto original=apply(0);camera_distance=1.5f;camera_height=30;
    const auto adjusted=apply(3);const auto restored=Read<Vec3>(memory,view+0x10);
    bool ok=Finite(original)&&Finite(adjusted)&&std::abs(original.x+100)<.01f && std::abs(adjusted.x+150)<.01f &&
        std::abs(adjusted.y+60)<.01f && std::abs(adjusted.z+450)<.01f && restored.x==100&&restored.y==20&&restored.z==300;
    Write(memory,view+0x150,uint32_t(0x3622C0));Write(memory,view+0x170,uint16_t(1));
    std::array<uint8_t,0x90> saved{};std::memcpy(saved.data(),memory+view,saved.size());
    camera_mode=4;X86Context call{};call.esp=GuestTestMemory::stack;Write(memory,call.esp+4,view);
    sub_003623B0(call,memory);
    const bool free_ok=std::memcmp(saved.data(),memory+view,saved.size())==0 &&
        Read<uint32_t>(memory,view+0x150)==0x3622C0 && Read<uint16_t>(memory,view+0x170)==1 && Finite(Read<Vec3>(memory,view+0xC0));
    Write(memory,0x12B6FAC,uint32_t(0x260000));Write(memory,0x12B6760,uint32_t(0x261000));
    Write(memory,0x261008,uint32_t(0x262000));Write(memory,0x262030,uint32_t(1));
    Write(memory,0x262034,uint32_t(0x263000));Write(memory,0x263000,uint8_t(3));
    Write(memory,0x263004,uint32_t(0x0F000000));Write(memory,0x263008,uint32_t(0x264000));
    for(uint32_t i=0;i<3;++i) Write(memory,0x264000+i*4,0x265000+i*16);
    Write(memory,0x265000,std::array<float,4>{-100,-100,50,1});
    Write(memory,0x265010,std::array<float,4>{100,-100,50,1});
    Write(memory,0x265020,std::array<float,4>{0,100,50,1});
    auto collide=[&](float x) {
        Write(memory,0x270000,std::array<float,4>{x,0,0,1});Write(memory,0x270010,std::array<float,4>{x,0,100,1});
        Write(memory,0x270020,std::array<float,4>{});
        X86Context query{};query.esp=GuestTestMemory::stack;
        Write(memory,query.esp+4,uint32_t(0x270020));Write(memory,query.esp+8,uint32_t(0x270000));
        Write(memory,query.esp+12,uint32_t(0x270010));__imp__sub_002FE650(query,memory);
        return std::pair{query.eax,Read<Vec3>(memory,0x270020)};
    };
    const auto hit=collide(0),miss=collide(500);
    const bool collision=hit.first==1 && std::abs(hit.second.z+52)<.05f && miss.first==0 &&
        miss.second.x==0 && miss.second.y==0 && miss.second.z==0;
    // Exercita o hook orbital inteiro, com o D0 do handle propositalmente incorreto.
    Write(memory,0x262030,uint32_t(0));
    const auto prior_target=orbit_target;
    const float prior_heading=camera_heading.load();
    bool orbit_ok=true;
    camera_mode=5;camera_distance=1;camera_height=30;
    for(int i=0;i<4;++i) {
        const float heading=i*1.5707963268f;
        Write(memory,record+0x2C,heading);flying=false;orbiting=false;
        X86Context orbital{};orbital.esp=GuestTestMemory::stack;Write(memory,orbital.esp+4,view);
        sub_003623B0(orbital,memory);
        const auto look=Read<Vec3>(memory,view+0x70);
        const auto matrix=Read<std::array<float,16>>(memory,view+0x90);
        orbit_ok=orbit_ok&&orbiting&&camera_live.load()&&orbit_target.x==100&&orbit_target.y==350&&orbit_target.z==200&&
            std::abs(orbit_yaw-heading)<.001f&&Finite(look)&&std::hypot(look.x,look.z)>.5f&&
            std::all_of(matrix.begin(),matrix.end(),[](float value){return std::isfinite(value);})&&
            Read<uint32_t>(memory,view+0x150)==0x3622C0&&orbital.esp==GuestTestMemory::stack+4;
    }
    orbit_target=prior_target;camera_heading=prior_heading;
    camera_mode=previous;camera_distance=distance;camera_height=height;camera_live=false;flying=false;orbiting=false;
    LOGR("dev selftest: camera native matrices/restore=%d freecam/restore=%d",ok,free_ok);
    LOGR("dev selftest: orbit native collision=%d hit=%u correctionZ=%.2f miss=%u",collision,hit.first,hit.second.z,miss.first);
    LOGR("dev selftest: orbit player pose native getter/negative height=%d",pose_ok);
    LOGR("dev selftest: orbit mode5 native matrices/cardinal target/height=%d",orbit_ok);
    return ok&&free_ok&&collision&&pose_ok&&orbit_ok;
}
}
