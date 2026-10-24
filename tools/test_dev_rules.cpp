#include "../src/common/dev_rules.h"
#include <array>
#include <cstdio>
#include <limits>

int main() {
    using namespace dev_rules;
    if (!RelativeAsset("data/text.dar")) return 1;
    if (RelativeAsset("../save.dat") || RelativeAsset("C:/outside") || RelativeAsset("/outside")) return 2;
    if (RelativeAsset("data/../../save") || RelativeAsset("data/foo:bar")) return 3;
    if (!CheatAddress(0x10000,4) || CheatAddress(0,4) || CheatAddress(0x3FFFFFF,4) || CheatAddress(0x10000,3)) return 4;
    if (WidthMask(1)!=255 || WidthMask(2)!=65535) return 5;
    std::array<float,4> frames{.5f,-.5f,1,-1};
    MixAudio(frames,.5f,0,false);
    if(frames!=std::array<float,4>{.25f,-.25f,.5f,-.5f}) return 6;
    MixAudio(frames,1,1,false);
    if(frames[0]!=0 || frames[1]!=-.25f) return 7;
    MixAudio(frames,1,0,true);
    if(frames!=std::array<float,4>{0,0,0,0}) return 8;
    if(AudioSample(std::numeric_limits<float>::quiet_NaN(),1)!=0 || AudioSample(3,1)!=1) return 9;
    if(MoveAxis(true,true)!=0 || MoveAxis(true,false)!=32767 || MoveAxis(false,true)!=-32767) return 10;
    if(MouseAxis(1000,.1f,false)!=32767 || MouseAxis(1000,.1f,true)!=-32767) return 11;
    if(!ReadOnlyGameOpen(0x120089,1,0) || ReadOnlyGameOpen(0x120089,1,0x1000) || ReadOnlyGameOpen(0x120089,3,0)) return 12;
    for (const uint32_t mutation : {2u,4u,16u,256u,0x10000u,0x40000u,0x80000u,0x02000000u,0x10000000u,0x40000000u})
        if(ReadOnlyGameOpen(0x120089|mutation,1,0)) return 13;
    MenuChord chord;
    if(chord.Update(0) || chord.Update(1u<<6) || !chord.Update(MenuChord::mask)) return 14;
    if(chord.Update(MenuChord::mask) || chord.Update(1u<<6) || chord.Update(MenuChord::mask)) return 15;
    if(chord.Update(0) || !chord.Update(MenuChord::mask)) return 16;
    chord.Update(0);
    if(chord.Update((1u<<4)|(1u<<7))) return 20;
    if(chord.Update((1u<<4)|(1u<<5))) return 21;
    if(!chord.Update((1u<<6)|(1u<<7))) return 22;
    ReleaseGate back;
    MenuKeyPulse pulse;
    pulse.Press(1000,false);
    if(!pulse.Active(1000) || !pulse.Active(1099) || pulse.Active(1100) || pulse.back) return 23;
    pulse.Press(2000,true);
    if(!pulse.Active(2000) || !pulse.back) return 24;
    pulse.Clear();if(pulse.Active(2000)) return 25;
    if(back.Filter(false,true) || back.Filter(true,true) || back.Filter(true,true)) return 17;
    if(back.Filter(true,false) || !back.Filter(true,true)) return 18;
    if(back.Filter(false,false) || back.Filter(true,true)) return 19;
    std::puts("Dev: caminhos de mods, limites de cheats, volume/balanco e entradas passaram");
}
