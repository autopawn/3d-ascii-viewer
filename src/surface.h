#pragma once

#include "trigonometry.h"

#include <stdio.h>

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

void surface_clear(struct surface *surface);

void surface_draw_triangle(struct surface *surface, struct triangle triangle);

void surface_print(FILE *fp, const struct surface *surface);

void surface_printw(const struct surface *surface);
