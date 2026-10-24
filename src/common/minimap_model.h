#pragma once
#include "minimap_math.h"
#include <cstdint>
#include <array>
#include <cstring>
#include <span>
#include <vector>

namespace cod_map {
struct Triangle { Point a,b,c; };
template<class T> inline bool Read(std::span<const uint8_t> bytes,size_t offset,T& value) {
    if(offset>bytes.size() || sizeof(T)>bytes.size()-offset) return false;
    std::memcpy(&value,bytes.data()+offset,sizeof(T));return true;
}
inline std::vector<Triangle> Model(std::span<const uint8_t> bytes) {
    std::vector<Triangle> result;
    uint32_t count=0,offset=0,signature=0,length=0;
    if(!Read(bytes,0,signature) || signature!=0xFFFF0003 || !Read(bytes,32,count) || !count || count>256 ||
       !Read(bytes,36,offset) || !Read(bytes,44,length) || length!=bytes.size()) return {};
    size_t mesh=offset;
    uint32_t matrix_offset=0;
    std::array<float,16> matrix{};
    if(!Read(bytes,8,matrix_offset) || !Read(bytes,matrix_offset,matrix)) return {};
    for(float value:matrix) if(!std::isfinite(value)) return {};
    for(uint32_t m=0;m<count;++m) {
        uint32_t next=0,packet_offset=0,vertices=0,indices=0,vertex_offset=0;
        if(!Read(bytes,mesh,next) || next<0xc0 || mesh+next>bytes.size() ||
           !Read(bytes,mesh+8,packet_offset)) return {};
        const size_t packet=mesh+packet_offset;
        if(!Read(bytes,packet+12,vertices) || !Read(bytes,packet+28,indices) ||
           !Read(bytes,packet+44,vertex_offset) || vertices>65536 || indices>100000) return {};
        const size_t data=packet+vertex_offset;
        const size_t index_data=(data+size_t(vertices)*36+3)&~size_t(3);
        if(index_data+size_t(indices)*2>mesh+next) return {};
        std::vector<Point> points;
        for(uint32_t v=0;v<vertices;++v) {
            float x=0,y=0,z=0;
            if(!Read(bytes,data+v*36,x) || !Read(bytes,data+v*36+4,y) || !Read(bytes,data+v*36+8,z) ||
               !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || std::abs(x)>1e7f || std::abs(z)>1e7f) return {};
            points.push_back({x*matrix[0]+y*matrix[4]+z*matrix[8]+matrix[12],
                              x*matrix[2]+y*matrix[6]+z*matrix[10]+matrix[14]});
        }
        uint16_t previous[2]{};
        for(uint32_t i=0;i<indices;++i) {
            uint16_t index=0;if(!Read(bytes,index_data+i*2,index) || index>=vertices) return {};
            if(i>=2) {
                const auto a=points[previous[0]],b=points[previous[1]],c=points[index];
                if(std::abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))>.001f)
                    result.push_back({a,b,c});
            }
            previous[0]=previous[1];previous[1]=index;
        }
        mesh+=next;
    }
    return result;
}
}
