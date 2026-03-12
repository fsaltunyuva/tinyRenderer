#include <iostream>
#include <vector>
#include <cstdlib>
#include "tgaimage.h"
#include "furgl.h"
#include "Furmatrix.h"
#include "Furmodel.h"

struct RandomShader : public IShader {
    const Furmodel &model;
    TGAColor color;

    RandomShader(const Furmodel &m) : model(m) {
    }

    Furvec4 vertex(int iface, int nthvert) override {
        int v_idx = model.face(iface)[nthvert];
        Furvec3 v = model.vert(v_idx);

        Furvec3 eye_coords = multiply_with_w(ModelView, v);
        float w = eye_coords.z * Perspective.data[3][2] + Perspective.data[3][3];
        Furvec3 clip_coords = multiply_with_w(Perspective, eye_coords);

        return {clip_coords.x, clip_coords.y, clip_coords.z, w};
    }

    std::pair<bool, TGAColor> fragment(Furvec3 bar) override {
        return {false, color};
    }
};

struct PhongShader : public IShader {
    const Furmodel &model;
    Furvec3 varying_nrm[3];
    Furvec3 light_dir = normalized(Furvec3(1, 5, 1));

    PhongShader(const Furmodel &m) : model(m) {}

    Furvec4 vertex(int iface, int nthvert) override {
        int v_idx = model.face(iface)[nthvert];
        varying_nrm[nthvert] = model.normal(iface, nthvert);

        Furvec3 v = model.vert(v_idx);
        Furvec3 eye_coords = multiply_with_w(ModelView, v);
        float w = eye_coords.z * Perspective.data[3][2] + Perspective.data[3][3];

        Furvec3 p = multiply_with_w(Perspective, eye_coords);
        return {p.x, p.y, p.z, w};
    }

    std::pair<bool, TGAColor> fragment(Furvec3 bar) override {
        Furvec3 bn = normalized(varying_nrm[0]*bar.x + varying_nrm[1]*bar.y + varying_nrm[2]*bar.z);
        float diffuse = std::max(0.f, bn * light_dir);
        Furvec3 r = normalized(bn * (bn * light_dir) * 2.f - light_dir);
        float specular = std::pow(std::max(0.f, r.z), 32);

        TGAColor color{255, 255, 255, 255};
        for (int i=0; i<3; i++) {
            float intensity = 0.1f + 0.6f * diffuse + 0.7f * specular;
            color.bgra[i] = (uint8_t)std::min(255.f, 255.f * intensity);
        }
        return {false, color};
    }
};

int main(int argc, char **argv) {
    const int width = 800;
    const int height = 800;

    Furvec3 eye(-1, 0, 2);
    Furvec3 center(0, 0, 0);
    Furvec3 up(0, 1, 0);

    lookat(eye, center, up);
    init_perspective((eye - center).norm());
    init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
    init_zbuffer(width, height);

    TGAImage framebuffer(width, height, TGAImage::RGB);

    Furmodel model("models/diablo3_pose.obj");
    PhongShader shader(model);

    for (int i = 0; i < model.nfaces(); i++) {
        Triangle screen_coords;
        Furvec3 clip_z;

        for (int j = 0; j < 3; j++) {
            Furvec4 clip_vert = shader.vertex(i, j);

            screen_coords[j] = multiply_with_w(ViewPort, Furvec3(clip_vert.x, clip_vert.y, clip_vert.z));

            if (j == 0) clip_z.x = clip_vert.z;
            else if (j == 1) clip_z.y = clip_vert.z;
            else clip_z.z = clip_vert.z;
        }

        // shader.color = TGAColor{
        //     (uint8_t) (std::rand() % 255), (uint8_t) (std::rand() % 255), (uint8_t) (std::rand() % 255), 255
        // };
        rasterize(screen_coords, &clip_z, shader, framebuffer);
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
