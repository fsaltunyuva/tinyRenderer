#include "tgaimage.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

struct vec3
{
    float x, y, z;
};

struct ivec3
{
    int x, y, z;
};

// constexpr does computation at compile time rather than run time
constexpr TGAColor white = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green = {0, 255, 0, 255};
constexpr TGAColor red = {0, 0, 255, 255};
constexpr TGAColor blue = {255, 128, 64, 255};
constexpr TGAColor yellow = {0, 200, 255, 255};

void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color)
{
    bool steep = std::abs(ax - bx) < std::abs(ay - by);
    if (steep)
    { // if the line is steep, we transpose the image
        std::swap(ax, ay);
        std::swap(bx, by);
    }

    if (ax > bx)
    { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }

    int y = ay;
    int ierror = 0;
    for (int x = ax; x <= bx; x++)
    {
        if (steep) // if transposed, de−transpose
            framebuffer.set(y, x, color);
        else
            framebuffer.set(x, y, color);
        ierror += 2 * std::abs(by - ay);
        y += (by > ay ? 1 : -1) * (ierror > bx - ax);
        ierror -= 2 * (bx - ax) * (ierror > bx - ax);
    }
}

pair<int, int> project(const vec3 &v, int width, int height)
{
    int x = (int)((v.x + 1.f) * width * 0.5f);
    int y = (int)((v.y + 1.f) * height * 0.5f);
    return {x, y};
}

static vector<string> split(const string &s, char delim)
{
    vector<string> elems;
    stringstream ss(s);
    string item;
    while (getline(ss, item, delim))
    {
        if (!item.empty())
            elems.push_back(item);
    }
    return elems;
}

void readObjFile(const string &fileName, vector<vec3> &vertices, vector<ivec3> &faces)
{
    ifstream in(fileName);
    if (!in.is_open())
    {
        cerr << "[ERR] Cannot open OBJ: " << fileName << "\n";
        return;
    }
    cerr << "[OK] Opened OBJ: " << fileName << "\n";

    if (!in)
    {
        cerr << "Cannot open: " << fileName << "\n";
        return;
    }

    string line;
    while (getline(in, line))
    {
        if (line.size() < 2)
            continue;

        // vertex line: "v x y z"
        if (line.rfind("v ", 0) == 0)
        {
            auto parts = split(line, ' ');
            if (parts.size() >= 4)
            {
                vertices.push_back({stof(parts[1]), stof(parts[2]), stof(parts[3])});
            }
        }
        // face line: "f a/b/c d/b/c e/b/c" OR "f a d e"
        else if (line.rfind("f ", 0) == 0)
        {
            auto parts = split(line, ' ');
            if (parts.size() >= 4)
            {
                auto p1 = split(parts[1], '/');
                auto p2 = split(parts[2], '/');
                auto p3 = split(parts[3], '/');

                ivec3 f{};
                f.x = stoi(p1[0]);
                f.y = stoi(p2[0]);
                f.z = stoi(p3[0]);
                faces.push_back(f);
            }
        }
    }
}

// 2d cross product / 2
double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy)
{
    return .5 * ((by - ay) * (bx + ax) + (cy - by) * (cx + bx) + (ay - cy) * (ax + cx));
}

void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color)
{
    // line(ax, ay, bx, by, framebuffer, color);
    // line(bx, by, cx, cy, framebuffer, color);
    // line(cx, cy, ax, ay, framebuffer, color);

#pragma region Modern Rasterization
    int bbminx = std::min(std::min(ax, bx), cx); // bounding box for the triangle
    int bbminy = std::min(std::min(ay, by), cy); // defined by its top left and bottom right corners
    int bbmaxx = std::max(std::max(ax, bx), cx);
    int bbmaxy = std::max(std::max(ay, by), cy);

    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);
    if (total_area < 1)
        return; // backface culling + discarding triangles that cover less than a pixel

#pragma omp parallel for
    for (int x = bbminx; x <= bbmaxx; x++)
    {
        for (int y = bbminy; y <= bbmaxy; y++)
        {
            double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta = signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma = signed_triangle_area(x, y, ax, ay, bx, by) / total_area;

            // outside the triangle
            if (alpha < 0 || beta < 0 || gamma < 0)
                continue;

            framebuffer.set(x, y, color);
        }
    }

#pragma endregion
}

int main()
{
    constexpr int width = 800;
    constexpr int height = 800;
    TGAImage framebuffer(width, height, TGAImage::RGB);

    vector<vec3> vertices;
    vector<ivec3> faces;

    readObjFile("../../models/diablo3_pose.obj", vertices, faces);

    for (int i = 0; i < (int)faces.size(); i++)
    {
        vec3 v0 = vertices[faces[i].x - 1];
        vec3 v1 = vertices[faces[i].y - 1];
        vec3 v2 = vertices[faces[i].z - 1];

        auto [x0, y0] = project(v0, width, height);
        auto [x1, y1] = project(v1, width, height);
        auto [x2, y2] = project(v2, width, height);

        // line(x0, y0, x1, y1, framebuffer, red);
        // line(x1, y1, x2, y2, framebuffer, red);
        // line(x2, y2, x0, y0, framebuffer, red);

        TGAColor randomColor = {(uint8_t)(rand() % 255), (uint8_t)(rand() % 255), (uint8_t)(rand() % 255), 255};
        triangle(x0, y0, x1, y1, x2, y2, framebuffer, randomColor);
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}