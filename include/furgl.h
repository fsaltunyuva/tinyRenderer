#pragma once

#include "tgaimage.h"
#include "Furvec3.h"
#include "Furmatrix.h"
#include <vector>

struct Furvec4 {
    float x, y, z, w;
};

struct IShader {
    virtual ~IShader() {
    }

    virtual Furvec4 vertex(int iface, int nthvert) = 0;

    virtual std::pair<bool, TGAColor> fragment(Furvec3 bar) = 0;
};

extern Furmatrix ModelView;
extern Furmatrix ViewPort;
extern Furmatrix Perspective;

void lookat(const Furvec3 eye, const Furvec3 center, const Furvec3 up);

void init_perspective(const double f);

void init_viewport(const int x, const int y, const int w, const int h);

void init_zbuffer(const int width, const int height);

typedef Furvec3 Triangle[3];

void rasterize(const Triangle &pts, const Furvec3 *clip_z, IShader &shader, TGAImage &framebuffer);
