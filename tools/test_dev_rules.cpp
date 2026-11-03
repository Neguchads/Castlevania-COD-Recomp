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
    TitleResetChord reset;
    const uint16_t start_back=TitleResetChord::mask;
    // Sem os dois gatilhos, ou sem Start/Back, nada acontece.
    if(reset.Update(start_back,255,0,0) || reset.Update(1u<<4,255,255,0) || reset.Update(start_back,127,255,0)) return 26;
    if(reset.Update(start_back,255,255,10)!=TitleResetChord::Started) return 27;
    if(reset.Update(start_back,255,255,11.9)!=TitleResetChord::Idle) return 28;
    if(reset.Update(start_back,255,255,12)!=TitleResetChord::Fire) return 29;
    // Dispara uma vez por pressao; soltar antes do tempo zera a contagem.
    if(reset.Update(start_back,255,255,20)!=TitleResetChord::Idle) return 30;
    reset.Update(0,0,0,21);
    if(reset.Update(start_back,255,255,22)!=TitleResetChord::Started || reset.Update(start_back,0,255,23.5)) return 31;
    if(reset.Update(start_back,255,255,24)!=TitleResetChord::Started || reset.Update(start_back,255,255,25)) return 32;
    // Teclado: setas viram direcional (cima/baixo/esquerda/direita = bits 0..3), Start/Back mantem bits 4/5.
    if(KeyboardDigitalBit(24)!=1 || KeyboardDigitalBit(25)!=2 || KeyboardDigitalBit(26)!=4 || KeyboardDigitalBit(27)!=8) return 33;
    if(KeyboardDigitalBit(8)!=(1u<<4) || KeyboardDigitalBit(9)!=(1u<<5)) return 34;
    if(KeyboardDigitalBit(0)!=0 || KeyboardDigitalBit(20)!=0 || KeyboardDigitalBit(23)!=0 || KeyboardDigitalBit(28)!=0) return 35;
    // Configuracao antiga com Enter no Start migra para P; nova ou remapeada fica como esta.
    if(MigratedStartKey(13,false,13,'P')!='P' || MigratedStartKey(13,true,13,'P')!=13 ||
       MigratedStartKey('K',false,13,'P')!='K') return 36;
    // F3 segurado: so o primeiro WM_KEYDOWN captura; repeticoes (bit 30) nao.
    if(!FirstKeyDown(0x003D0001) || FirstKeyDown(0x403D0001) || !FirstKeyDown(0x203D0001)) return 37;
    // Analogico direito: inverter troca o sinal sem estourar no -32768 e mantem o centro em 0.
    if(InvertAxis(12000,true)!=-12000 || InvertAxis(-32768,true)!=32767 || InvertAxis(32767,true)!=-32767 ||
       InvertAxis(0,true)!=0 || InvertAxis(-5000,false)!=-5000) return 38;
    // Diagnostico do Back: so a borda de subida conta, segurar nao repete.
    if(!RisingBit(0,0x20,0x20) || RisingBit(0x20,0x20,0x20) || RisingBit(0x20,0,0x20) || RisingBit(0,0x10,0x20)) return 39;
    std::puts("Dev: caminhos de mods, limites de cheats, volume/balanco e entradas passaram");
}
