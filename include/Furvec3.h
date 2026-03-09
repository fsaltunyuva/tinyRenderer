#pragma once
#include "Furmatrix.h"

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

    Furvec3 operator*(Furmatrix other)
    {
        Furvec3 result;
        // hardcoded for 3x3 * 3x1
        result.x = this->x * other.data[0][0] + this->y * other.data[0][1] + this->z * other.data[0][2];
        result.y = this->x * other.data[1][0] + this->y * other.data[1][1] + this->z * other.data[1][2];
        result.z = this->x * other.data[2][0] + this->y * other.data[2][1] + this->z * other.data[2][2];
        return result;
    }
};
