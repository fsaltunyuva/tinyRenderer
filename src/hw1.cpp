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