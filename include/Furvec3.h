#pragma once
#include "Furmatrix.h"

// TODO: PLEASE USE BETTER STRUCTURES FOR MATH
class Furvec3
{
public:
    float x, y, z;
    Furvec3()
    {
        this->x = 0;
        this->y = 0;
        this->z = 0;
    }

    Furvec3(float x, float y, float z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    Furvec3 operator-(const Furvec3& other) const
    {
        return Furvec3(this->x - other.x, this->y - other.y, this->z - other.z);
    }

    Furvec3 operator*(Furmatrix other)
    {
        Furvec3 result;
        // hardcoded for 3x3 * 3x1
        result.x = this->x * other.data[0][0] + this->y * other.data[0][1] + this->z * other.data[0][2];
        result.y = this->x * other.data[1][0] + this->y * other.data[1][1] + this->z * other.data[1][2];
        result.z = this->x * other.data[2][0] + this->y * other.data[2][1] + this->z * other.data[2][2];
        return result;
    }

    Furvec3 operator/(float other) const
    {
        Furvec3 result;
        result.x = this->x / other;
        result.y = this->y / other;
        result.z = this->z / other;
        return result;
    }

    float norm() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    static Furvec3 cross(const Furvec3& a, const Furvec3& b) {
        return Furvec3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }
};

inline Furvec3 normalized(const Furvec3& v) {
    float n = v.norm();
    if (n > 0) return v / n;
    return Furvec3(0, 0, 0); // Avoid division by zero
}

inline Furvec3 cross(const Furvec3& a, const Furvec3& b) {
    return Furvec3::cross(a, b);
}

// with 4x4 matrix with w = 1
inline Furvec3 multiply_with_w(const Furmatrix& m, const Furvec3& v) {
    float x = v.x * m.data[0][0] + v.y * m.data[0][1] + v.z * m.data[0][2] + 1.f * m.data[0][3];
    float y = v.x * m.data[1][0] + v.y * m.data[1][1] + v.z * m.data[1][2] + 1.f * m.data[1][3];
    float z = v.x * m.data[2][0] + v.y * m.data[2][1] + v.z * m.data[2][2] + 1.f * m.data[2][3];
    float w = v.x * m.data[3][0] + v.y * m.data[3][1] + v.z * m.data[3][2] + 1.f * m.data[3][3];

    return Furvec3(x/w, y/w, z/w);
}
