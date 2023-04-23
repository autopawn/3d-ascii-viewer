#pragma once

#include <math.h>

typedef struct
{
    float x, y, z;
} vec3;

struct triangle
{
    union
    {
        vec3 pts[3];
        struct
        {
            vec3 p1, p2, p3;
        };
    };
    char color;
};

static inline float vec3_mag(vec3 v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static inline vec3 vec3_normalize(vec3 v)
{
    float mag = vec3_mag(v);
    v.x /= mag;
    v.y /= mag;
    v.z /= mag;
    return v;
}
