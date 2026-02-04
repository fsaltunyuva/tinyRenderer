#include "tgaimage.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

struct vec3{
  float x, y, z;
};

struct ivec3{
  int x, y, z;
};

// constexpr does computation at compile time rather than run time
constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

// Attempt 1
// x(t) = ax + t * (bx - ax)), and same logic for y(t)
void line1(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    for (float t=0.; t<1.; t+=.02) {
        int x = std::round( ax + (bx-ax)*t );
        int y = std::round( ay + (by-ay)*t );
        framebuffer.set(x, y, color);
    }
}

// Attempt 2 (Define t as function of x)
// t(x) = (x-ax)/(bx - ax)
void line2(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    for (int x=ax; x<=bx; x++) {
        float t = (x-ax) / static_cast<float>(bx-ax);
        int y = std::round( ay + (by-ay)*t );
        framebuffer.set(x, y, color);
    }
}

// Attempt 2-1 (Solve green line)
void line2_fixed(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    if (ax>bx) { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }
    for (int x=ax; x<=bx; x++) {
        float t = (x-ax) / static_cast<float>(bx-ax);
        int y = std::round( ay + (by-ay)*t );
        framebuffer.set(x, y, color);
    }
}

// Attempt 3 (If steep, transpose)
void line3(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool steep = std::abs(ax-bx) < std::abs(ay-by);
    if (steep) { // if the line is steep, we transpose the image
        std::swap(ax, ay);
        std::swap(bx, by);
    }

    if (ax>bx) { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }

    for (int x=ax; x<=bx; x++) {
        float t = (x-ax) / static_cast<float>(bx-ax);
        int y = std::round( ay + (by-ay)*t );

        if (steep) // if transposed, de−transpose
            framebuffer.set(y, x, color);
        else
            framebuffer.set(x, y, color);
    }
}

// Attempt 3-1 (x always increase by one, therefore y(x) always increases (by-ay)/(bx-ax)
// y0 = ay
// y1 = y0 + slope
// y2 = y1 + slope
// This eliminates division and multiplication in each iteration
void line3_optimization1(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool steep = std::abs(ax-bx) < std::abs(ay-by);
    if (steep) { // if the line is steep, we transpose the image
        std::swap(ax, ay);
        std::swap(bx, by);
    }

    if (ax>bx) { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }

    float y = ay;
    for (int x=ax; x<=bx; x++) {
        if (steep) // if transposed, de−transpose
            framebuffer.set(y, x, color);
        else
            framebuffer.set(x, y, color);

        y += (by-ay) / static_cast<float>(bx-ax);
    }
}

// Attempt 3-2 (we are already making y int when writing to framebuffer make it int)
// Use error to determine jump on y
void line3_optimization2(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool steep = std::abs(ax-bx) < std::abs(ay-by);
    if (steep) { // if the line is steep, we transpose the image
        std::swap(ax, ay);
        std::swap(bx, by);
    }

    if (ax>bx) { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }

    int y = ay;
    float error = 0;
    for (int x=ax; x<=bx; x++) {
        if (steep) // if transposed, de−transpose
            framebuffer.set(y, x, color);
        else
            framebuffer.set(x, y, color);
        error += std::abs(by-ay)/static_cast<float>(bx-ax);
        if (error>.5) {
            y += by > ay ? 1 : -1;
            error -= 1.;
        }
    }
}

// Attempt 3-3 (getting rid of float)
// ierror = error * 2 * (bx - ax)
void line3_optimization3(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool steep = std::abs(ax-bx) < std::abs(ay-by);
    if (steep) { // if the line is steep, we transpose the image
        std::swap(ax, ay);
        std::swap(bx, by);
    }

    if (ax>bx) { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }

    int y = ay;
    int ierror = 0;
    for (int x=ax; x<=bx; x++) {
        if (steep) // if transposed, de−transpose
            framebuffer.set(y, x, color);
        else
            framebuffer.set(x, y, color);
        ierror += 2 * std::abs(by-ay);
        if (ierror > bx - ax) {
            y += by > ay ? 1 : -1;
            ierror -= 2 * (bx-ax);
        }
    }
}

// Attempt 4 (branchless rasterization)
void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool steep = std::abs(ax-bx) < std::abs(ay-by);
    if (steep) { // if the line is steep, we transpose the image
        std::swap(ax, ay);
        std::swap(bx, by);
    }

    if (ax>bx) { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }

    int y = ay;
    int ierror = 0;
    for (int x=ax; x<=bx; x++) {
        if (steep) // if transposed, de−transpose
            framebuffer.set(y, x, color);
        else
            framebuffer.set(x, y, color);
        ierror += 2 * std::abs(by-ay);
        y += (by > ay ? 1 : -1) * (ierror > bx - ax);
        ierror -= 2 * (bx-ax)   * (ierror > bx - ax);
    }
}

pair<int,int> project(const vec3& v, int width, int height) {
    int x = static_cast<int>((v.x + 1.f) * width  * 0.5f);
    int y = static_cast<int>((v.y + 1.f) * height * 0.5f);
    return {x, y};
}

void readObjFile(string fileName, vector<vec3> &vertices, vector<ivec3> &faces);

int main(int argc, char** argv) {
    constexpr int width  = 800;
    constexpr int height = 800;
    TGAImage framebuffer(width, height, TGAImage::RGB);

    // int ax =  7, ay =  3;
    // int bx = 12, by = 37;
    // int cx = 62, cy = 53;

    // Attempt 1
    // line1(ax, ay, bx, by, framebuffer, blue);
    // line1(cx, cy, bx, by, framebuffer, green);
    // // These two overlaps but some yellow pixels still remains (bc. of order of args)
    // // Also there are gaps in red line because of sample size (+=0.02)
    // line1(cx, cy, ax, ay, framebuffer, yellow);
    // line1(ax, ay, cx, cy, framebuffer, red);

    // Attempt 2
    // Resolves red line gaps, but green line disappears (cx > bx, loop doesn't start)
    // Also blue line has gaps because num of samples is insufficient
    // line2(ax, ay, bx, by, framebuffer, blue);
    // line2(cx, cy, bx, by, framebuffer, green);
    // line2(cx, cy, ax, ay, framebuffer, yellow);
    // line2(ax, ay, cx, cy, framebuffer, red);

    // Attempt 2-1
    // Solves green line, but not the blue line (still insufficient sampling for steep lines)
    // line2_fixed(ax, ay, bx, by, framebuffer, blue);
    // line2_fixed(cx, cy, bx, by, framebuffer, green);
    // line2_fixed(cx, cy, ax, ay, framebuffer, yellow);
    // line2_fixed(ax, ay, cx, cy, framebuffer, red);

    // Attempt 3
    // line3(ax, ay, bx, by, framebuffer, blue);
    // line3(cx, cy, bx, by, framebuffer, green);
    // line3(cx, cy, ax, ay, framebuffer, yellow);
    // line3(ax, ay, cx, cy, framebuffer, red);

    // Optimizations
    // Attempt 3-1
    // line3_optimization1(ax, ay, bx, by, framebuffer, blue);
    // line3_optimization1(cx, cy, bx, by, framebuffer, green);
    // line3_optimization1(cx, cy, ax, ay, framebuffer, yellow);
    // line3_optimization1(ax, ay, cx, cy, framebuffer, red);

    // Attempt 3-2 (error rate)
    // line3_optimization2(ax, ay, bx, by, framebuffer, blue);
    // line3_optimization2(cx, cy, bx, by, framebuffer, green);
    // line3_optimization2(cx, cy, ax, ay, framebuffer, yellow);
    // line3_optimization2(ax, ay, cx, cy, framebuffer, red);
     
    // line(ax, ay, bx, by, framebuffer, blue);
    // line(cx, cy, bx, by, framebuffer, green);
    // line(cx, cy, ax, ay, framebuffer, yellow);
    // line(ax, ay, cx, cy, framebuffer, red);

    // framebuffer.set(ax, ay, white);
    // framebuffer.set(bx, by, white);
    // framebuffer.set(cx, cy, white);

    // framebuffer.write_tga_file("framebuffer.tga");

    vector<vec3> vertices;
    vector<ivec3> faces;
    readObjFile("../models/diablo3_pose.obj", vertices, faces);

    for (int i = 0; i < faces.size(); i++) {
        vec3 v0 = vertices[faces[i].x - 1];
        vec3 v1 = vertices[faces[i].y - 1];
        vec3 v2 = vertices[faces[i].z - 1];

        auto [x0, y0] = project(v0, width, height);
        auto [x1, y1] = project(v1, width, height);
        auto [x2, y2] = project(v2, width, height);

        line(x0, y0, x1, y1, framebuffer, red);
        line(x1, y1, x2, y2, framebuffer, red);
        line(x2, y2, x0, y0, framebuffer, red);
    }

    framebuffer.write_tga_file("framebuffer.tga");

    return 0;
}

void split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

vector<string> split(const string &s, char delim) {
    vector<string> elems;
    split(s, delim, elems);
    return elems;
}

void readObjFile(string fileName, vector<vec3> &vertices, vector<ivec3> &faces){
    ifstream ReadObjFile(fileName);
    string input;

    while (getline(ReadObjFile, input)) {
        if (input[0] == 'v'){
            vector<string> strings;
            strings = split(input, ' ');
            if (strings[0] == "v"){
                vec3 tempvec3;
                tempvec3.x = stof(strings[1]);
                tempvec3.y = stof(strings[2]);
                tempvec3.z = stof(strings[3]);
                vertices.push_back(tempvec3);
            }
        }

        if (input[0] == 'f'){
            vector<string> strings;
            strings = split(input, ' ');
            ivec3 tempvec3;

            for (int i = 1; i < 4; i++){
                vector<string> values;
                values = split(strings[i], '/');

                if (i == 0) tempvec3.x = stoi(values[0]);
                if (i == 1) tempvec3.y = stoi(values[0]);
                if (i == 2) tempvec3.z = stoi(values[0]);
            }

            faces.push_back(tempvec3);
        }
    }

    ReadObjFile.close();
}