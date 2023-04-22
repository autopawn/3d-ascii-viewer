#pragma once

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
