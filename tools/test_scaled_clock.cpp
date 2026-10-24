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
    if(dev::GuestDeadline(-10000,100,900)!=1000100) return 6;
    if(dev::GuestDeadline(1000,100,900)!=10100 || dev::GuestDeadline(800,100,900)!=100) return 7;
    if(dev::GuestDeadline(INT64_MIN,100,900)!=INT64_MAX) return 8;
    if(!dev::BlockPlayerDamage(true,6,0,-10) || dev::BlockPlayerDamage(false,6,0,-10) ||
       dev::BlockPlayerDamage(true,6,1,-10) || dev::BlockPlayerDamage(true,6,0,10) || dev::BlockPlayerDamage(true,7,0,-10)) return 9;
    std::puts("Clock: continuidade 1x/2x/0.5x, frequencia, deadlines e overflow passaram");
}
