#include "tgaimage.h"

using namespace std;

// constexpr does computation at compile time rather than run time
constexpr int width = 128;
constexpr int height = 128;
constexpr TGAColor white   = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green   = {  0, 255,   0, 255};
constexpr TGAColor red     = {  0,   0, 255, 255};
constexpr TGAColor blue    = {255, 128,  64, 255};
constexpr TGAColor yellow  = {  0, 200, 255, 255};

// branchless rasterization
void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool steep = abs(ax-bx) < abs(ay-by);
    if (steep) { // if the line is steep, we transpose the image
        swap(ax, ay);
        swap(bx, by);
    }

    if (ax>bx) { // make it left−to−right
        swap(ax, bx);
        swap(ay, by);
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
        if (ierror > bx - ax)
        {
            y += by > ay ? 1 : -1;
            ierror -= 2 * (bx - ax);
        }
    }
}

void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer, TGAColor color)
{
    line(ax, ay, bx, by, framebuffer, color);
    line(bx, by, cx, cy, framebuffer, color);
    line(cx, cy, ax, ay, framebuffer, color);

    // make it ay <= by <= cy
    if (ay > by)
    {
        swap(ax, bx);
        swap(ay, by);
    }
    if (ay > cy)
    {
        swap(ax, cx);
        swap(ay, cy);
    }
    if (by > cy)
    {
        swap(bx, cx);
        swap(by, cy);
    }

    // use green triangle as example bc. it satisfies ay <= by <= cy directly
    for (int y = ay; y <= cy; y++)
    {
        float t1 = (float)(y - ay) / (cy - ay);
        int rightx = ax + t1 * (cx - ax);

        float t2;
        int leftx;

        if (y < by) // bottom half
        {
            t2 = (float)(y - ay) / (by - ay);
            leftx = ax + t2 * (bx - ax);
        }
        else{ // upper half
            t2 = (float)(y - by) / (cy - by);
            leftx = bx + t2 * (cx - bx);
        }

        line(leftx, y, rightx, y, framebuffer, color);
    }
}

int main(int argc, char** argv) {
    TGAImage framebuffer(width, height, TGAImage::RGB);

    triangle(7, 45, 35, 100, 45, 60, framebuffer, red);
    triangle(120, 35, 90, 5, 45, 110, framebuffer, white);
    triangle(115, 83, 80, 90, 85, 120, framebuffer, green);

    framebuffer.write_tga_file("framebuffer.tga");

    return 0;
}