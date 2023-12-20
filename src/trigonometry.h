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

    if (mag == 0)
        return (vec3){0, 0, 0};

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

static inline vec3 vec3_add(vec3 a, vec3 b)
{
    vec3 res;

    res.x = a.x + b.x;
    res.y = a.y + b.y;
    res.z = a.z + b.z;
    return res;
}

static inline vec3 vec3_sub(vec3 a, vec3 b)
{
    vec3 res;

    res.x = a.x - b.x;
    res.y = a.y - b.y;
    res.z = a.z - b.z;
    return res;
}

static inline float vec3_dot_product(vec3 a, vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline float vec3_cos_similarity(vec3 a, vec3 b, float a_mag, float b_mag)
{
    return vec3_dot_product(a, b) / (a_mag * b_mag);
}

static inline vec3 vec3_cross_product(vec3 a, vec3 b)
{
    vec3 prod;

    prod.x = a.y * b.z - a.z * b.y;
    prod.y = a.z * b.x - a.x * b.z;
    prod.z = a.x * b.y - a.y * b.x;

    return prod;
}

vec3 get_bounding_box_center(const vec3 *A, int n);

float get_max_dist(const vec3 *A, int n, vec3 p);
