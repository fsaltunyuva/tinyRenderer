#include <complex>
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
    const TGAImage &normal_map;
    const TGAImage &diffuse_map;
    const TGAImage &specular_map;

    Furvec3 varying_nrm[3];
    Furvec3 light_dir = normalized(Furvec3(1, 5, 1));
    Furvec3 varying_uv[3];
    Furvec3 varying_diff[3];
    Furvec3 varying_spec[3];

    PhongShader(const Furmodel &m, const TGAImage &nm, const TGAImage &diff, const TGAImage &spec) : model(m), normal_map(nm), diffuse_map(diff), specular_map(spec) {}

    Furvec4 vertex(int iface, int nthvert) override {
        int v_idx = model.face(iface)[nthvert];
        varying_nrm[nthvert] = model.normal(iface, nthvert);
        varying_uv[nthvert] = model.uv(iface, nthvert); // fetch from model for nthvert vertex and iface face
        varying_diff[nthvert] = model.uv(iface, nthvert); // fetch from model for nthvert vertex and iface face
        varying_spec[nthvert] = model.uv(iface, nthvert); // fetch from model for nthvert vertex and iface face

        Furvec3 v = model.vert(v_idx);
        Furvec3 eye_coords = multiply_with_w(ModelView, v);
        float w = eye_coords.z * Perspective.data[3][2] + Perspective.data[3][3];

        Furvec3 p = multiply_with_w(Perspective, eye_coords);
        return {p.x, p.y, p.z, w};
    }

    std::pair<bool, TGAColor> fragment(Furvec3 bar) override {
        // Normal Mapping
        Furvec3 uv = varying_uv[0] * bar.x + varying_uv[1] * bar.y + varying_uv[2] * bar.z; // interpolation with barycentric weights

        int tex_x = std::min(normal_map.width() - 1, std::max(0, (int)(uv.x * normal_map.width())));
        int tex_y = std::min(normal_map.height() - 1, std::max(0, (int)(uv.y * normal_map.height())));

        TGAColor c = normal_map.get(tex_x, tex_y);

        Furvec3 n(
            c.bgra[2] / 255.f * 2.f - 1.f, // Red -> X
            c.bgra[1] / 255.f * 2.f - 1.f, // Green -> Y
            c.bgra[0] / 255.f * 2.f - 1.f  // Blue -> Z
        );
        n = normalized(n);

        // Diffuse Mapping
        Furvec3 diff = varying_diff[0] * bar.x + varying_diff[1] * bar.y + varying_diff[2] * bar.z; // interpolation with barycentric weights

        int tex_x_diff = std::min(diffuse_map.width() - 1, std::max(0, (int)(diff.x * diffuse_map.width())));
        int tex_y_diff = std::min(diffuse_map.height() - 1, std::max(0, (int)(diff.y * diffuse_map.height())));

        TGAColor diffuse_color = diffuse_map.get(tex_x_diff, tex_y_diff);

        // Specular Mapping
        Furvec3 spec = varying_spec[0] * bar.x + varying_spec[1] * bar.y + varying_spec[2] * bar.z; // interpolation with barycentric weights

        int tex_x_spec= std::min(specular_map.width() - 1, std::max(0, (int)(spec.x * specular_map.width())));
        int tex_y_spec = std::min(specular_map.height() - 1, std::max(0, (int)(spec.y * specular_map.height())));

        TGAColor specular_color = specular_map.get(tex_x_spec, tex_y_spec);

        Furvec3 bn = normalized(varying_nrm[0]*bar.x + varying_nrm[1]*bar.y + varying_nrm[2]*bar.z); // interpolating normals using barycentric weights
        // float diffuse = std::max(0.f, bn * light_dir);
        float diffuse = std::max(0.f, n * light_dir);
        Furvec3 r = normalized(bn * (bn * light_dir) * 2.f - light_dir);
        float specular = std::pow(std::max(0.f, r.z), 32);

        TGAColor color{
            static_cast<std::uint8_t>(std::min(255, diffuse_color.bgra[0] + specular_color.bgra[0])),
            static_cast<std::uint8_t>(std::min(255, diffuse_color.bgra[1] + specular_color.bgra[1])),
            static_cast<std::uint8_t>(std::min(255, diffuse_color.bgra[2] + specular_color.bgra[2])),
            255
        };

        // for (int i=0; i<3; i++) {
        //     float intensity = 0.1f + 0.6f * (color.bgra[i] / 255.f) + 0.7f * specular;
        //     color.bgra[i] = static_cast<std::uint8_t>(std::min(255.f, 255.f * intensity));
        // }

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
    TGAImage normal_map;
    normal_map.read_tga_file("models/diablo3_pose_nm.tga");
    normal_map.flip_vertically(); // Aligns the image Y-axis with texture V-coordinates

    TGAImage diffuse_map;
    diffuse_map.read_tga_file("models/diablo3_pose_diffuse.tga");
    diffuse_map.flip_vertically(); // Aligns the image Y-axis with texture V-coordinates

    TGAImage specular_map;
    specular_map.read_tga_file("models/diablo3_pose_spec.tga");
    specular_map.flip_vertically(); // Aligns the image Y-axis with texture V-coordinates

    Furmodel model("models/diablo3_pose.obj");
    PhongShader shader(model, normal_map, diffuse_map, specular_map);

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
