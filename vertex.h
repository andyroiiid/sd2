//
// Created by Andrew Huang on 1/23/2022.
//

#ifndef SD2_VERTEX_H
#define SD2_VERTEX_H

#include <cstdint>

struct Vec2 {
    float x;
    float y;
};

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Rgba8 {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct VertexPCU {
    Vec3 position;
    Rgba8 color;
    Vec2 uv;
};

#endif //SD2_VERTEX_H
