#pragma once

#include <math.h>

typedef struct
{
    float x, y, z;
} vec3;

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

static inline vec3 vec3_neg(vec3 v)
{
    v.x = -v.x;
    v.y = -v.y;
    v.z = -v.z;
    return v;
}

static inline float vec3_cos_similarity(vec3 a, vec3 b, float a_mag, float b_mag)
{
    return (a.x * b.x + a.y * b.y + a.z * b.z) / (a_mag * b_mag);
}
