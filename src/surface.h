#pragma once

#include "trigonometry.h"

#include <stdio.h>
#include <stdbool.h>

struct pixel
{
    float z;
    char c;
    int material;
};

struct surface
{
    // Size in characters
    unsigned int size_y, size_x;

    // Logical size
    float logical_size_x, logical_size_y;
    // Logical size of each character
    float dx, dy;

    struct pixel *pixels;
};

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

struct surface *surface_init(unsigned int size_x, unsigned int size_y, float logical_size_x,
    float logical_size_y);

void surface_free(struct surface *surface);

void surface_clear(struct surface *surface);

void surface_draw_triangle(struct surface *surface, struct triangle tri, bool inverted_orientation,
        char c, int material);

void surface_print(FILE *fp, const struct surface *surface);

void surface_printw(const struct surface *surface);

vec3 triangle_normal(const struct triangle *tri);
