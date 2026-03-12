#include "furgl.h"
#include <algorithm>
#include <limits>

Furmatrix ModelView(4, 4);
Furmatrix ViewPort(4, 4);
Furmatrix Perspective(4, 4);
std::vector<float> zbuffer;

void lookat(const Furvec3 eye, const Furvec3 center, const Furvec3 up) {
    Furvec3 n = normalized(eye - center);
    Furvec3 l = normalized(cross(up, n));
    Furvec3 m = normalized(cross(n, l));

    Furmatrix Mtranspose(4, 4);
    Mtranspose.addData({
        l.x, l.y, l.z, 0,
        m.x, m.y, m.z, 0,
        n.x, n.y, n.z, 0,
        0, 0, 0, 1
    });

    Furmatrix C(4, 4);
    C.addData({
        1, 0, 0, -center.x,
        0, 1, 0, -center.y,
        0, 0, 1, -center.z,
        0, 0, 0, 1
    });

    ModelView = Mtranspose * C;
}

void init_perspective(const double f) {
    Perspective.addData({
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, -1.0f / (float) f, 1
    });
}

void init_viewport(const int x, const int y, const int w, const int h) {
    ViewPort.addData({
        w / 2.f, 0, 0, x + w / 2.f,
        0, h / 2.f, 0, y + h / 2.f,
        0, 0, 1, 0,
        0, 0, 0, 1
    });
}

void init_zbuffer(const int width, const int height) {
    zbuffer.assign(width * height, -std::numeric_limits<float>::max());
}

static float cross_2d(float x0, float y0, float x1, float y1, float x2, float y2) {
    return (x1 - x0) * (y2 - y0) - (y1 - y0) * (x2 - x0);
}

void rasterize(const Triangle &pts, const Furvec3 *clip_z, IShader &shader, TGAImage &framebuffer) {
    // Bounding box
    int bbminx = std::max(0, (int) std::min({pts[0].x, pts[1].x, pts[2].x}));
    int bbminy = std::max(0, (int) std::min({pts[0].y, pts[1].y, pts[2].y}));
    int bbmaxx = std::min(framebuffer.width() - 1, (int) std::max({pts[0].x, pts[1].x, pts[2].x}));
    int bbmaxy = std::min(framebuffer.height() - 1, (int) std::max({pts[0].y, pts[1].y, pts[2].y}));

    float area = cross_2d(pts[0].x, pts[0].y, pts[1].x, pts[1].y, pts[2].x, pts[2].y);
    if (std::abs(area) < 1) return;

    for (int x = bbminx; x <= bbmaxx; x++) {
        for (int y = bbminy; y <= bbmaxy; y++) {
            float w0 = cross_2d(pts[1].x, pts[1].y, pts[2].x, pts[2].y, (float) x, (float) y) / area;
            float w1 = cross_2d(pts[2].x, pts[2].y, pts[0].x, pts[0].y, (float) x, (float) y) / area;
            float w2 = 1.0f - w0 - w1;

            if (w0 < 0 || w1 < 0 || w2 < 0) continue;

            float z = w0 * clip_z->x + w1 * clip_z->y + w2 * clip_z->z;

            int idx = x + y * framebuffer.width();
            if (z > zbuffer[idx]) {
                auto [discard, color] = shader.fragment(Furvec3(w0, w1, w2));
                if (!discard) {
                    zbuffer[idx] = z;
                    framebuffer.set(x, y, color);
                }
            }
        }
    }
}
