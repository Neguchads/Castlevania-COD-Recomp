#pragma once
#include <algorithm>
#include <cmath>
#include <optional>

namespace cod_map {
struct Point { float x=0,y=0; };
struct Bounds { float left=0,top=0,right=0,bottom=0; };
inline Point HeadingUp(Point delta,float heading) {
    return {delta.x*std::cos(heading)-delta.y*std::sin(heading),
            delta.x*std::sin(heading)+delta.y*std::cos(heading)};
}
// Mesma escala, rotacoes e deslocamentos do cursor nativo sub_003464B0.
inline std::optional<Point> NativePoint(Point world,int room_x,int room_y,int rotation,bool model=false) {
    if(!std::isfinite(world.x) || !std::isfinite(world.y) || rotation<0 || rotation>3) return std::nullopt;
    const float x=world.x*(model ? 1.0f : .08f),y=world.y*(model ? 1.0f : .08f);
    constexpr Point offsets[]{{0,0},{0,-320},{-320,-320},{0,0}};
    const Point rotated[]{ {x,y},{y,-x},{-x,-y},{-y,x} };
    return Point{rotated[rotation].x-room_x*16+offsets[rotation].x,
                 rotated[rotation].y+room_y*16+offsets[rotation].y};
}
struct Projection {
    Point offset;
    float scale=1;
    Point Apply(Point point) const { return {offset.x+point.x*scale,offset.y+point.y*scale}; }
};
inline std::optional<Projection> Fit(Bounds bounds,Point size,float padding) {
    const float width=bounds.right-bounds.left,height=bounds.bottom-bounds.top;
    if(!std::isfinite(width) || !std::isfinite(height) || !std::isfinite(bounds.left) ||
       !std::isfinite(bounds.top) || !std::isfinite(size.x) || !std::isfinite(size.y) ||
       !std::isfinite(padding) || width<=0 || height<=0 || padding<0 ||
       size.x<=2*padding || size.y<=2*padding) return std::nullopt;
    const float scale=std::min((size.x-2*padding)/width,(size.y-2*padding)/height);
    return Projection{{(size.x-width*scale)*.5f-bounds.left*scale,
                       (size.y-height*scale)*.5f-bounds.top*scale},scale};
}
}
