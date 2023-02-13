#pragma once

#include <stdio.h>

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

struct pixel
{
    float z;
    char color;
};

struct surface
{
    unsigned int size_y, size_x;

    struct pixel *pixels;
};

struct surface *surface_init(unsigned int size_x, unsigned int size_y);

void surface_free(struct surface *surface);

void surface_draw_triangle(struct surface *surface, struct triangle triangle);

void surface_print(FILE *fp, const struct surface *surface);
