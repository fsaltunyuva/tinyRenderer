#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include "Furvec3.h"

class Furmodel {
public:
    std::vector<Furvec3> verts;
    std::vector<std::vector<int> > faces;

    Furmodel(const char *filename) {
        std::ifstream in(filename);
        if (in.fail()) {
            std::cerr << "Could not open file: " << filename << std::endl;
            return;
        }
        std::string line;
        while (!in.eof()) {
            std::getline(in, line);
            if (!line.compare(0, 2, "v ")) {
                std::stringstream ss(line.substr(2));
                float x, y, z;
                ss >> x >> y >> z;
                verts.push_back(Furvec3(x, y, z));
            } else if (!line.compare(0, 2, "f ")) {
                std::vector<int> f;
                int idx, tmp;
                std::stringstream ss(line.substr(2));
                while (ss >> idx) {
                    f.push_back(--idx);
                    if (ss.peek() == '/') {
                        ss.ignore();
                        if (ss.peek() != '/') {
                            ss >> tmp;
                        }
                        if (ss.peek() == '/') {
                            ss.ignore();
                            ss >> tmp;
                        }
                    }
                }
                faces.push_back(f);
            }
        }
        std::cout << "[OK] OBJ Loaded: " << verts.size() << " vertices, " << faces.size() << " faces." << std::endl;
    }

    Furvec3 normal(int iface, int nthvert) const {
        int i0 = faces[iface][0];
        int i1 = faces[iface][1];
        int i2 = faces[iface][2];
        Furvec3 a = verts[i0];
        Furvec3 b = verts[i1];
        Furvec3 c = verts[i2];
        return normalized(cross(c - a, b - a));
    }

    int nverts() const { return (int) verts.size(); }
    int nfaces() const { return (int) faces.size(); }
    Furvec3 vert(int i) const { return verts[i]; }
    std::vector<int> face(int idx) const { return faces[idx]; }
};
