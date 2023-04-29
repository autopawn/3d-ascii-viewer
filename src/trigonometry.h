#pragma once

#include <math.h>

typedef struct
{
    float x, y, z;
} vec3;

typedef struct
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
} triangle;

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

static inline vec3 vec3_rotate_y(float cos, float sin, vec3 v)
{
    float x = v.x * cos - v.z * sin;
    float z = v.x * sin + v.z * cos;
    v.x = x;
    v.z = z;
    return v;
}

static inline vec3 vec3_rotate_x(float cos, float sin, vec3 v)
{
    float y = v.y * cos - v.z * sin;
    float z = v.y * sin + v.z * cos;
    v.y = y;
    v.z = z;
    return v;
}

static inline float vec3_cos_similarity(vec3 a, vec3 b, float a_mag, float b_mag)
{
    return (a.x * b.x + a.y * b.y + a.z * b.z) / (a_mag * b_mag);
}

static inline vec3 triangle_normal(const triangle *tri)
{
    vec3 v1, v2, normal;

    v1.x = tri->p3.x - tri->p1.x;
    v1.y = tri->p3.y - tri->p1.y;
    v1.z = tri->p3.z - tri->p1.z;

    v2.x = tri->p2.x - tri->p1.x;
    v2.y = tri->p2.y - tri->p1.y;
    v2.z = tri->p2.z - tri->p1.z;

    normal.x = v1.y * v2.z - v1.z * v2.y;
    normal.y = v1.z * v2.x - v1.x * v2.z;
    normal.z = v1.x * v2.y - v1.y * v2.x;

    return vec3_normalize(normal);
}
