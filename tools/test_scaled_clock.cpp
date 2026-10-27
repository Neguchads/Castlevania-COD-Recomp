#include "../src/common/scaled_clock.h"
#include "../src/common/game_cheat_rules.h"
#include <cstdio>

int main() {
    dev::ScaledClock clock(1000);
    if(clock.Now(2000)!=1000) return 1;
    clock.SetRate(2000,2);
    if(clock.Now(2000)!=1000 || clock.Now(3000)!=3000) return 2;
    clock.SetRate(3000,.5);
    if(clock.Now(3000)!=3000 || clock.Now(5000)!=4000) return 3;
    clock.SetRate(5000,1);
    if(clock.Now(6000)!=5000) return 4;
    clock.SetRate(6000,std::numeric_limits<double>::quiet_NaN());
    if(clock.Rate()!=1 || clock.Now(7000)!=6000) return 5;
    // speedhack ate 4x; acima disso e abaixo de 0,5x fica no limite
    dev::ScaledClock fast(0);
    fast.SetRate(0,4);
    if(fast.Rate()!=dev::kMaxSpeed || fast.Now(1000)!=4000) return 14;
    fast.SetRate(1000,10);
    if(fast.Rate()!=4 || dev::kMaxSpeed!=4) return 15;
    fast.SetRate(1000,.1);
    if(fast.Rate()!=dev::kMinSpeed || dev::kMinSpeed!=.5) return 16;
    if(dev::GuestDeadline(-10000,100,900)!=1000100) return 6;
    if(dev::GuestDeadline(1000,100,900)!=10100 || dev::GuestDeadline(800,100,900)!=100) return 7;
    if(dev::GuestDeadline(INT64_MIN,100,900)!=INT64_MAX) return 8;
    if(!dev::BlockPlayerDamage(true,6,0,-10) || dev::BlockPlayerDamage(false,6,0,-10) ||
       dev::BlockPlayerDamage(true,6,1,-10) || dev::BlockPlayerDamage(true,6,0,10) || dev::BlockPlayerDamage(true,7,0,-10)) return 9;
    // Pausa do menu dev: o tempo do jogo para e volta sem salto; trocar a velocidade pausado nao o move.
    dev::ScaledClock paused(0);
    paused.SetPaused(1000,true);
    if(paused.Now(1000)!=1000 || paused.Now(9000)!=1000) return 10;
    paused.SetRate(9000,2);
    if(paused.Now(12000)!=1000) return 11;
    paused.SetPaused(12000,false);
    if(paused.Now(12000)!=1000 || paused.Now(13000)!=3000) return 12;
    paused.SetPaused(13000,false);
    if(paused.Now(14000)!=5000) return 13;
    std::puts("Clock: continuidade 1x/2x/0.5x, frequencia, deadlines, overflow e pausa passaram");
}
