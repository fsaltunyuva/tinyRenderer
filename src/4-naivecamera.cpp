#include "tgaimage.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include "Furmatrix.h"
#include "Furvec3.h"

using namespace std;

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

tuple<int, int> project(const Furvec3 &v, int width, int height)
{
    int x = (int)((v.x + 1.f) * width * 0.5f);
    int y = (int)((v.y + 1.f) * height * 0.5f);
    //int z = (int)((v.z + 1.f) * 255./2); // 0-255 (0 for far, 255 for near, buffer initialized as 0s)

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

void readObjFile(const string &fileName, vector<Furvec3> &vertices, vector<ivec3> &faces)
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


Furvec3 rot(Furvec3 v) {
    constexpr double a = M_PI/6;
    Furmatrix Ry(3,3);
    vector<float> rotationMatrix; // y
    // The worst code structure for math in the history
    rotationMatrix.push_back(cos(a));
    rotationMatrix.push_back(0);
    rotationMatrix.push_back(sin(a));

    rotationMatrix.push_back(0);
    rotationMatrix.push_back(1);
    rotationMatrix.push_back(0);

    rotationMatrix.push_back(-sin(a));
    rotationMatrix.push_back(0);
    rotationMatrix.push_back(cos(a));

    Ry.addData(rotationMatrix);
    return v*Ry;
}

Furvec3 perspective(Furvec3 v)
{
    constexpr double c = 3.;
    return v / (1 - v.z/c);
}

// 2d cross product / 2
double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy)
{
    return .5 * ((by - ay) * (bx + ax) + (cy - by) * (cx + bx) + (ay - cy) * (ax + cx));
}

void triangle(int ax, int ay, float az, int bx, int by, float bz, int cx, int cy, float cz, vector<vector<float>> &depthBuffer, TGAImage &framebuffer, TGAColor color)
{
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
            if (alpha < 0 || beta < 0 || gamma < 0) continue;
            float z = alpha * az + beta * bz + gamma * cz;
            if (z <= depthBuffer[x][y]) continue;
            depthBuffer[x][y] = z;
            framebuffer.set(x, y, color);
        }
    }

}

int main()
{
    constexpr int width = 800;
    constexpr int height = 800;
    TGAImage framebuffer(width, height, TGAImage::RGB);
    // TGAImage depthbuffer(width, height, TGAImage::GRAYSCALE);
    vector<vector<float>> depthbuffer(width, vector<float>(height, -numeric_limits<float>::max()));

    vector<Furvec3> vertices;
    vector<ivec3> faces;

    readObjFile("models/diablo3_pose.obj", vertices, faces);

    for (int i = 0; i < (int)faces.size(); i++)
    {
        Furvec3 pv0 = perspective(rot(vertices[faces[i].x - 1]));
        Furvec3 pv1 = perspective(rot(vertices[faces[i].y - 1]));
        Furvec3 pv2 = perspective(rot(vertices[faces[i].z - 1]));

        auto [x0, y0] = project(pv0, width, height);
        auto [x1, y1] = project(pv1, width, height);
        auto [x2, y2] = project(pv2, width, height);

        float z0 = pv0.z;
        float z1 = pv1.z;
        float z2 = pv2.z;

        TGAColor randomColor = {(uint8_t)(rand() % 255), (uint8_t)(rand() % 255), (uint8_t)(rand() % 255), 255};
        triangle(x0, y0, z0, x1, y1, z1, x2, y2, z2, depthbuffer, framebuffer, randomColor);
    }

    framebuffer.write_tga_file("framebuffer.tga");

    // Depth buffer
    TGAImage depthImg(width, height, TGAImage::GRAYSCALE);
    for (int x = 0; x < width; x++)
        for (int y = 0; y < height; y++) {
            float normalized = (depthbuffer[x][y] + 1.f) * 0.5f * 255.f;
            unsigned char val = (unsigned char)max(0.f, min(255.f, normalized));
            depthImg.set(x, y, {val});
        }
    depthImg.write_tga_file("depthbuffer.tga");
    return 0;
}