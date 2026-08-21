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
    std::vector<Furvec3> norms;
    std::vector<Furvec3> uvs;
    std::vector<std::vector<int>> faces;     // vertex indices per face
    std::vector<std::vector<int>> facet_vt;  // texture coordinate indices per face
    std::vector<std::vector<int>> facet_vn;  // normal indices per face

    Furmodel(const char *filename) {
        std::ifstream in(filename);
        if (in.fail()) {
            std::cerr << "Could not open file: " << filename << std::endl;
            return;
        }
        std::string line;
        while (std::getline(in, line)) {
            if (!line.compare(0, 3, "vn ") || !line.compare(0, 3, "vn\t")) {
                std::stringstream ss(line.substr(2));
                float x, y, z;
                ss >> x >> y >> z;
                norms.push_back(normalized(Furvec3(x, y, z)));
            } else if (!line.compare(0, 3, "vt ") || !line.compare(0, 3, "vt\t")) {
                std::stringstream ss(line.substr(2));
                float u, v, w = 0.f;
                ss >> u >> v;
                if (ss >> w) {}
                uvs.push_back(Furvec3(u, v, w));
            } else if (!line.compare(0, 2, "v ") || !line.compare(0, 2, "v\t")) {
                std::stringstream ss(line.substr(2));
                float x, y, z;
                ss >> x >> y >> z;
                verts.push_back(Furvec3(x, y, z));
            } else if (!line.compare(0, 2, "f ") || !line.compare(0, 2, "f\t")) {
                std::vector<int> f_v, f_vt, f_vn;
                std::stringstream ss(line.substr(2));
                std::string token;
                while (ss >> token) {
                    int v = -1, vt = -1, vn = -1;
                    std::stringstream token_ss(token);
                    std::string part;

                    if (std::getline(token_ss, part, '/')) {
                        if (!part.empty()) v = std::stoi(part) - 1;
                    }
                    if (std::getline(token_ss, part, '/')) {
                        if (!part.empty()) vt = std::stoi(part) - 1;
                    }
                    if (std::getline(token_ss, part, '/')) {
                        if (!part.empty()) vn = std::stoi(part) - 1;
                    }

                    f_v.push_back(v);
                    f_vt.push_back(vt);
                    f_vn.push_back(vn);
                }
                faces.push_back(f_v);
                facet_vt.push_back(f_vt);
                facet_vn.push_back(f_vn);
            }
        }
        std::cout << "[OK] OBJ Loaded: " << verts.size() << " vertices, "
                  << faces.size() << " faces, "
                  << norms.size() << " normals, "
                  << uvs.size() << " texture coords." << std::endl;
    }

    Furvec3 normal(int iface, int nthvert) const {
        if (iface >= 0 && iface < (int)facet_vn.size() && nthvert >= 0 && nthvert < (int)facet_vn[iface].size()) {
            int idx = facet_vn[iface][nthvert];
            if (idx >= 0 && idx < (int)norms.size()) {
                return norms[idx];
            }
        }
        // Fallback to geometric face normal if vn is not specified
        int i0 = faces[iface][0];
        int i1 = faces[iface][1];
        int i2 = faces[iface][2];
        Furvec3 a = verts[i0];
        Furvec3 b = verts[i1];
        Furvec3 c = verts[i2];
        return normalized(cross(c - a, b - a));
    }

    Furvec3 normal(int i) const {
        return norms[i];
    }

    Furvec3 uv(int iface, int nthvert) const {
        if (iface >= 0 && iface < (int)facet_vt.size() && nthvert >= 0 && nthvert < (int)facet_vt[iface].size()) {
            int idx = facet_vt[iface][nthvert];
            if (idx >= 0 && idx < (int)uvs.size()) {
                return uvs[idx];
            }
        }
        return Furvec3(0, 0, 0);
    }

    int nverts() const { return (int) verts.size(); }
    int nfaces() const { return (int) faces.size(); }
    int nnormals() const { return (int) norms.size(); }
    int ntex_coords() const { return (int) uvs.size(); }
    Furvec3 vert(int i) const { return verts[i]; }
    Furvec3 vert(int iface, int nthvert) const { return verts[faces[iface][nthvert]]; }
    std::vector<int> face(int idx) const { return faces[idx]; }
};
