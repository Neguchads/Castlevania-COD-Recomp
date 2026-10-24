#include "../src/common/menu_preferences.h"
#include <limits>
#include <cstdio>
int main() {
    if(dev::MenuContentScale(1120,760)!=1 || dev::MenuContentScale(560,380)!=.5f ||
       dev::MenuContentScale(2240,1520)!=2 || dev::MenuContentScale(1120,380)!=.5f) return 4;
    if(dev::MenuContentScale(0,760)!=1 || dev::MenuContentScale(std::numeric_limits<float>::quiet_NaN(),760)!=1) return 5;
    dev::MenuPreferences prefs;
    prefs.x=-100;prefs.y=std::numeric_limits<float>::infinity();
    prefs.width=10;prefs.height=std::numeric_limits<float>::quiet_NaN();prefs.opacity=0;
    prefs.accent={-1,2,std::numeric_limits<float>::quiet_NaN()};prefs.Sanitize();
    if(prefs.x!=0 || prefs.y!=40 || prefs.width!=640 || prefs.height!=760 || prefs.opacity!=.25f) return 1;
    if(prefs.accent[0]!=0 || prefs.accent[1]!=1 || prefs.accent[2]!=.2f) return 2;
    prefs=dev::MenuPreferences{};
    if(prefs.opacity!=.88f || prefs.width!=1120 || prefs.positioned) return 3;
    std::puts("Menu: geometria, valores invalidos, transparencia e restauracao passaram");
}
