#include <algorithm>
#include <complex>
#include <iostream>
#include <vector>
#include <cstdlib>
#include "tgaimage.h"
#include "furgl.h"
#include "Furmatrix.h"
#include "Furmodel.h"

struct DepthShader : public IShader {
    const Furmodel &model;
    TGAColor color;

    DepthShader(const Furmodel &m) : model(m) {}

    Furvec4 vertex(int iface, int nthvert) override {
        int v_idx = model.face(iface)[nthvert];
        Furvec3 v = model.vert(v_idx);

        Furvec3 eye_coords = multiply_with_w(ModelView, v);
        float w = eye_coords.z * Perspective.data[3][2] + Perspective.data[3][3];
        Furvec3 clip_coords = multiply_with_w(Perspective, eye_coords);

        return {clip_coords.x, clip_coords.y, clip_coords.z, w};
    }

    std::pair<bool, TGAColor> fragment(Furvec3 bar) override {
        return {false, color}; // no need for color
    }
};

struct PhongShader : public IShader {
    const Furmodel &model;
    const TGAImage &normal_map;
    const TGAImage &diffuse_map;
    const TGAImage &specular_map;
    TGAImage &shadow_frame;

    Furvec3 varying_nrm[3];
    Furvec3 light_dir = normalized(Furvec3(1, 5, 1));
    Furvec3 varying_vert[3]; // 3D positions of a vertex
    Furvec3 varying_uv[3];

    // Shadow Mapping
    const std::vector<float> &shadow_buffer;
    const Furmatrix &light_matrix;
    int width, height;

    PhongShader(const Furmodel &m, const TGAImage &nm, const TGAImage &diff, const TGAImage &spec, const std::vector<float> &sb, const Furmatrix &lm, int w, int h, TGAImage &sf)
                : model(m), normal_map(nm), diffuse_map(diff), specular_map(spec), shadow_buffer(sb), light_matrix(lm), width(w), height(h), shadow_frame(sf) {}

    Furvec4 vertex(int iface, int nthvert) override {
        int v_idx = model.face(iface)[nthvert];
        varying_nrm[nthvert] = model.normal(iface, nthvert);
        varying_uv[nthvert] = model.uv(iface, nthvert); // fetch from model for nthvert vertex and iface face

        Furvec3 v = model.vert(v_idx);
        varying_vert[nthvert] = v;
        Furvec3 eye_coords = multiply_with_w(ModelView, v);
        float w = eye_coords.z * Perspective.data[3][2] + Perspective.data[3][3];

        Furvec3 p = multiply_with_w(Perspective, eye_coords);
        return {p.x, p.y, p.z, w};
    }

    std::pair<bool, TGAColor> fragment(Furvec3 bar) override {
        // Tangent Space (TBN)
        Furvec3 e0 = varying_vert[1] - varying_vert[0];
        Furvec3 e1 = varying_vert[2] - varying_vert[0];
        Furvec3 u0 = varying_uv[1] - varying_uv[0];
        Furvec3 u1 = varying_uv[2] - varying_uv[0];

        float det = u0.x * u1.y - u1.x * u0.y;

        Furvec3 t = (e0 * u1.y - e1 * u0.y) * (1.0f / det);
        Furvec3 b = (e1 * u0.x - e0 * u1.x) * (1.0f / det);

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
        Furvec3 diff = varying_uv[0] * bar.x + varying_uv[1] * bar.y + varying_uv[2] * bar.z; // interpolation with barycentric weights

        int tex_x_diff = std::min(diffuse_map.width() - 1, std::max(0, (int)(diff.x * diffuse_map.width())));
        int tex_y_diff = std::min(diffuse_map.height() - 1, std::max(0, (int)(diff.y * diffuse_map.height())));

        TGAColor diffuse_color = diffuse_map.get(tex_x_diff, tex_y_diff);

        // Specular Mapping
        Furvec3 spec = varying_uv[0] * bar.x + varying_uv[1] * bar.y + varying_uv[2] * bar.z; // interpolation with barycentric weights

        int tex_x_spec= std::min(specular_map.width() - 1, std::max(0, (int)(spec.x * specular_map.width())));
        int tex_y_spec = std::min(specular_map.height() - 1, std::max(0, (int)(spec.y * specular_map.height())));

        TGAColor specular_color = specular_map.get(tex_x_spec, tex_y_spec);

        Furvec3 bn = normalized(varying_nrm[0]*bar.x + varying_nrm[1]*bar.y + varying_nrm[2]*bar.z); // interpolating normals using barycentric weights

        t = normalized(t);
        b = normalized(b);
        Furvec3 real_n = normalized(t * n.x + b * n.y + bn * n.z);

        float diffuse = std::max(0.f, real_n * light_dir);
        Furvec3 r = normalized(real_n * (real_n * light_dir) * 2.f - light_dir);
        float specular = std::pow(std::max(0.f, r.z), 32);

        Furvec3 p_model = varying_vert[0] * bar.x + varying_vert[1] * bar.y + varying_vert[2] * bar.z; // current triangle's 3D position
        Furvec3 p_light = multiply_with_w(light_matrix, p_model); // converting that position to light's perspective to check shadow, p_light.z gives depth from the light

        int idx = int(p_light.x) + int(p_light.y) * width; // fetch depth data

        float shadow = 1.0f; // 0 means it's in shadow
        if (idx >= 0 && idx < (int)shadow_buffer.size()) {
            if (shadow_buffer[idx] > p_light.z + 0.08f) { // occluded
                shadow = 0.3f; // ambient light for the shadow
            }
        }

        // With shading
        TGAColor color;
        for (int i = 0; i < 3; i++) {
            float channel = 10.f + (diffuse_color.bgra[i] * diffuse + 0.6f * specular_color.bgra[i] * specular) * shadow;
            color.bgra[i] = static_cast<std::uint8_t>(std::min(255.f, channel));
        }

        color.bgra[3] = 255;

        // Shadow Mask Drawing
        Furvec3 cam_coords = multiply_with_w(ModelView, p_model);
        Furvec3 clip_coords = multiply_with_w(Perspective, cam_coords);
        Furvec3 scr = multiply_with_w(ViewPort, clip_coords);

        uint8_t mask_val = (shadow < 1.0f) ? 0 : 255;
        TGAColor mask_color;
        mask_color.bgra[0] = mask_val;
        mask_color.bgra[1] = mask_val;
        mask_color.bgra[2] = mask_val;
        mask_color.bgra[3] = 255;

        int sx = std::clamp((int)scr.x, 0, width - 1);
        int sy = std::clamp((int)scr.y, 0, height - 1);
        shadow_frame.set(sx, sy, mask_color);

        return {false, color};
    }
};

struct ToonShader : public IShader {
    const Furmodel &model;
    const TGAImage &diffuse_map;

    Furvec3 varying_nrm[3];
    Furvec3 light_dir = normalized(Furvec3(1, 1, 3)); // bright forward-facing light
    Furvec3 varying_uv[3];

    ToonShader(const Furmodel &m, const TGAImage &diff) : model(m), diffuse_map(diff) {}

    Furvec4 vertex(int iface, int nthvert) override {
        int v_idx = model.face(iface)[nthvert];
        varying_nrm[nthvert] = model.normal(iface, nthvert);
        varying_uv[nthvert] = model.uv(iface, nthvert);

        Furvec3 v = model.vert(v_idx);
        Furvec3 eye_coords = multiply_with_w(ModelView, v);
        float w = eye_coords.z * Perspective.data[3][2] + Perspective.data[3][3];

        Furvec3 p = multiply_with_w(Perspective, eye_coords);
        return {p.x, p.y, p.z, w};
    }

    std::pair<bool, TGAColor> fragment(Furvec3 bar) override {
        Furvec3 n = normalized(varying_nrm[0] * bar.x + varying_nrm[1] * bar.y + varying_nrm[2] * bar.z);
        Furvec3 uv = varying_uv[0] * bar.x + varying_uv[1] * bar.y + varying_uv[2] * bar.z;

        int tex_x = std::min(diffuse_map.width() - 1, std::max(0, (int)(uv.x * diffuse_map.width())));
        int tex_y = std::min(diffuse_map.height() - 1, std::max(0, (int)(uv.y * diffuse_map.height())));

        TGAColor diffuse_color = diffuse_map.get(tex_x, tex_y);

        float raw_light = std::max(0.0f, n * light_dir);

        // Quantize light intensities
        float toon_intensity;
        if (raw_light > 0.8f) toon_intensity = 1.0f;
        else if (raw_light > 0.5f) toon_intensity = 0.75f;
        else if (raw_light > 0.25f) toon_intensity = 0.50f;
        else toon_intensity = 0.30f; // Ambient floor

        TGAColor color;
        for (int i = 0; i < 3; i++) {
            float channel = diffuse_color.bgra[i] * toon_intensity;
            color.bgra[i] = static_cast<std::uint8_t>(std::min(255.f, channel));
        }
        color.bgra[3] = 255;

        return {false, color};
    }
};

struct WorldPositionShader : public IShader {
    const Furmodel &model;
    std::vector<Furvec3> &world_positions;
    std::vector<bool> &has_surface;
    int width, height;

    Furvec3 varying_vert[3]; // 3D model vertices of the triangle
    Furvec3 varying_scr[3];  // Screen coordinates of the triangle

    WorldPositionShader(const Furmodel &m, std::vector<Furvec3> &wp, std::vector<bool> &hs, int w, int h) : model(m), world_positions(wp), has_surface(hs), width(w), height(h) {}

    Furvec4 vertex(int iface, int nthvert) override {
        int v_idx = model.face(iface)[nthvert];
        Furvec3 v = model.vert(v_idx);
        varying_vert[nthvert] = v;

        Furvec3 eye_coords = multiply_with_w(ModelView, v);
        float w = eye_coords.z * Perspective.data[3][2] + Perspective.data[3][3];

        Furvec3 p = multiply_with_w(Perspective, eye_coords);

        varying_scr[nthvert] = multiply_with_w(ViewPort, p);

        return {p.x, p.y, p.z, w};
    }
    std::pair<bool, TGAColor> fragment(Furvec3 bar) override {
        Furvec3 p_world = varying_vert[0] * bar.x + varying_vert[1] * bar.y + varying_vert[2] * bar.z; // current triangle's 3D position
        Furvec3 scr = varying_scr[0] * bar.x + varying_scr[1] * bar.y + varying_scr[2] * bar.z;

        // int casting with avoiding out of bounds memory
        int sx = std::clamp((int)(scr.x + 0.5f), 0, width - 1);
        int sy = std::clamp((int)(scr.y + 0.5f), 0, height - 1);

        world_positions[sx + sy * width] = p_world;
        has_surface[sx + sy * width] = true;
        return {false, TGAColor{}};
    }
};

Furvec3 sample_point_on_sphere(float radius = 2.5f) {
    constexpr double pi = 3.14159265358979323846;
    float z = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    float phi = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/(2 * pi)));
    float r = sqrt(1 - z * z);
    float x = r * cos(phi);
    float y = r * sin(phi);

    return Furvec3(x, y, z) * radius;
}

int main(int argc, char **argv) {
    const int width = 800;
    const int height = 800;

    Furvec3 eye(-1, 0, 2);
    Furvec3 center(0, 0, 0);
    Furvec3 up(0, 1, 0);

    Furmodel model("models/diablo3_pose.obj");

    // Pass 1: Render main camera view to capture world_positions and main zbuffer
    lookat(eye, center, up);
    init_perspective((eye - center).norm());
    init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
    init_zbuffer(width, height);

    std::vector<Furvec3> world_positions(width * height);
    std::vector<bool> has_surface(width * height, false);

    WorldPositionShader main_shader(model, world_positions, has_surface, width, height);
    TGAImage main_dump(width, height, TGAImage::RGB);

    for (int i = 0; i < model.nfaces(); i++) {
        Triangle screen_coords;
        Furvec3 clip_z;

        for (int j = 0; j < 3; j++) {
            Furvec4 clip_vert = main_shader.vertex(i, j);
            screen_coords[j] = multiply_with_w(ViewPort, Furvec3(clip_vert.x, clip_vert.y, clip_vert.z));

            if (j == 0) clip_z.x = clip_vert.z;
            else if (j == 1) clip_z.y = clip_vert.z;
            else clip_z.z = clip_vert.z;
        }

        rasterize(screen_coords, &clip_z, main_shader, main_dump);
    }

    extern std::vector<float> zbuffer;
    std::vector<float> main_zbuffer = zbuffer; // Save main view Z-buffer for SSAO and Sobel filter

    // Pass 2: Screen-Space Ambient Occlusion (SSAO)
    lookat(eye, center, up);
    init_perspective((eye - center).norm());
    init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
    Furmatrix MainMatrix = ViewPort * Perspective * ModelView;

    const int SSAO_SAMPLES = 16;
    TGAImage ssao_image(width, height, TGAImage::RGB);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = x + y * width; // 1D array of 2D coordinates

            // checking if the pixel is background
            if (!has_surface[idx]) {
                ssao_image.set(x, y, TGAColor{0, 0, 0, 255});
                continue;
            }

            Furvec3 p_world = world_positions[idx];
            float ssao_vis = 0.0f;

            for (int k = 0; k < SSAO_SAMPLES; k++) {
                float sample_radius = 0.02f + (float)rand() / RAND_MAX * 0.08f; // random radius
                Furvec3 p_sample = p_world + sample_point_on_sphere(sample_radius);

                // Projecting sample point into screen coordiantes
                Furvec3 p_scr = multiply_with_w(MainMatrix, p_sample);
                int u = (int)p_scr.x;
                int v = (int)p_scr.y;

                if (u >= 0 && u < width && v >= 0 && v < height) { // checking if u and v is inside the image frame and applying AO
                    int sample_idx = u + v * width;
                    float z_actual = main_zbuffer[sample_idx];
                    float z_sample = p_scr.z;

                    if (z_sample >= z_actual - 0.01f) {
                        ssao_vis += 1.0f;
                    }
                }
            }

            // averaging the ssao
            float factor = ssao_vis / static_cast<float>(SSAO_SAMPLES);
            uint8_t val = static_cast<uint8_t>(std::clamp(factor * 255.0f, 0.0f, 255.0f));
            ssao_image.set(x, y, TGAColor{val, val, val, 255});
        }
    }
    ssao_image.write_tga_file("ssao.tga");

    // Pass 3: Toon Shader Pass
    lookat(eye, center, up);
    init_perspective((eye - center).norm());
    init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
    init_zbuffer(width, height);

    TGAImage diffuse_map;
    diffuse_map.read_tga_file("models/diablo3_pose_diffuse.tga");
    diffuse_map.flip_vertically();

    TGAImage toon_frame(width, height, TGAImage::RGB);
    ToonShader toon_shader(model, diffuse_map);

    for (int i = 0; i < model.nfaces(); i++) {
        Triangle screen_coords;
        Furvec3 clip_z;

        for (int j = 0; j < 3; j++) {
            Furvec4 clip_vert = toon_shader.vertex(i, j);
            screen_coords[j] = multiply_with_w(ViewPort, Furvec3(clip_vert.x, clip_vert.y, clip_vert.z));

            if (j == 0) clip_z.x = clip_vert.z;
            else if (j == 1) clip_z.y = clip_vert.z;
            else clip_z.z = clip_vert.z;
        }

        rasterize(screen_coords, &clip_z, toon_shader, toon_frame);
    }

    // Pass 4: Sobel Filter on main_zbuffer to draw outlines
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int idx = x + y * width;
            if (!has_surface[idx]) continue;

            // 3x3 Z-buffer neighborhood
            float z00 = main_zbuffer[(x - 1) + (y - 1) * width];
            float z01 = main_zbuffer[(x) + (y - 1) * width];
            float z02 = main_zbuffer[(x + 1) + (y - 1) * width];

            float z10 = main_zbuffer[(x - 1) + (y) * width];
            float z12 = main_zbuffer[(x + 1) + (y) * width];

            float z20 = main_zbuffer[(x - 1) + (y + 1) * width];
            float z21 = main_zbuffer[(x) + (y + 1) * width];
            float z22 = main_zbuffer[(x + 1) + (y + 1) * width];

            // Sobel K_x kernel
            float Gx = (-1.f * z00 + 1.f * z02) +
                       (-2.f * z10 + 2.f * z12) +
                       (-1.f * z20 + 1.f * z22);

            // Sobel K_y kernel
            float Gy = (-1.f * z00 - 2.f * z01 - 1.f * z02) +
                       ( 1.f * z20 + 2.f * z21 + 1.f * z22);

            float gradient = std::sqrt(Gx * Gx + Gy * Gy);

            // Threshold for depth jump edge
            if (gradient > 0.1f) {
                toon_frame.set(x, y, TGAColor{255, 255, 255, 255}); // Black outline
            }
        }
    }

    toon_frame.write_tga_file("toon.tga");
    std::cout << "Rendered toon shading with Sobel depth outlines to toon.tga!\n";
    return 0;
}
