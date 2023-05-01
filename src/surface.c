#include "surface.h"

#include <assert.h>
#include <ncurses.h>
#include <stdlib.h>

static float mini(float a, float b)
{
    if (a < b)
        return a;
    return b;
}

static float maxi(float a, float b)
{
    if (a > b)
        return a;
    return b;
}

static bool triangle_orientation(const triangle *tri)
{
    return (tri->p2.x - tri->p1.x) * (tri->p3.y - tri->p2.y)
            < (tri->p3.x - tri->p2.x) * (tri->p2.y - tri->p1.y);
}

static triangle triangle_sort_by_x(triangle triangle)
{
    vec3 aux;
    for (unsigned int i = 0; i < 2; ++i)
    {
        for (unsigned int j = i + 1; j < 3; ++j)
        {
            if (triangle.pts[i].x > triangle.pts[j].x)
            {
                aux = triangle.pts[i];
                triangle.pts[i] = triangle.pts[j];
                triangle.pts[j] = aux;
            }
        }
    }
    return triangle;
}

struct surface *surface_init(unsigned int size_x, unsigned int size_y, float logical_size_x,
    float logical_size_y)
{
    struct surface *surface;

    if (!(surface = malloc(sizeof(*surface))))
    {
        fprintf(stderr, "ERROR: Memory allocation failure.\n");
        exit(1);
    }

    surface->size_x = size_x;
    surface->size_y = size_y;
    surface->logical_size_x = logical_size_x;
    surface->logical_size_y = logical_size_y;
    surface->dx = logical_size_x / size_x;
    surface->dy = logical_size_y / size_y;

    if (!(surface->pixels = malloc(size_y * size_x * sizeof(*surface->pixels))))
    {
        fprintf(stderr, "ERROR: Memory allocation failure.\n");
        exit(1);
    }
    surface_clear(surface);

    return surface;
}

void surface_clear(struct surface *surface)
{
    assert(surface);
    assert(surface->pixels);

    for (int i = 0; i < surface->size_y * surface->size_x; ++i)
    {
        surface->pixels[i].c = ' ';
        surface->pixels[i].z = INFINITY;
    }
}

void surface_free(struct surface *surface)
{
    free(surface->pixels);
    free(surface);
}

static inline int idx_x(const struct surface *surface, float x)
{
    float dx = surface->dx;
    return maxi(0, mini(surface->size_x - 1, (int) floorf(x / dx)));
}

static inline int idx_y(const struct surface *surface, float y)
{
    float dy = surface->dy;
    return maxi(0, mini(surface->size_y - 1, (int) floorf(y / dy)));
}

static inline float limit_y_1(const triangle *tri, float x)
{
    if (x <= tri->p1.x)
        return tri->p1.y;
    if (x >= tri->p3.x)
        return tri->p3.y;
    if (x <= tri->p2.x)
        return tri->p1.y + (tri->p2.y - tri->p1.y) * (x - tri->p1.x) / (tri->p2.x - tri->p1.x);
    return tri->p2.y + (tri->p3.y - tri->p2.y) * (x - tri->p2.x) / (tri->p3.x - tri->p2.x);
}

static inline float limit_y_2(const triangle *tri, float x)
{
    if (x <= tri->p1.x)
        return tri->p1.y;
    if (x >= tri->p3.x)
        return tri->p3.y;
    return tri->p1.y + (tri->p3.y - tri->p1.y) * (x - tri->p1.x) / (tri->p3.x - tri->p1.x);
}

static inline float triangle_depth(const struct surface *surface, const triangle *tri,
        vec3 normal, int xx, int yy)
{
    float dx = surface->dx;
    float dy = surface->dy;

    float x = (xx + 0.5) * dx;
    float y = (yy + 0.5) * dy;

    return tri->p1.z - (normal.x * (x - tri->p1.x) + normal.y * (y - tri->p1.y)) / normal.z;
}

void surface_draw_triangle(struct surface *surface, triangle tri, bool inverted_orientation,
        char c)
{
    if (triangle_orientation(&tri) != !inverted_orientation)
        return;

    vec3 normal = triangle_normal(&tri);

    tri = triangle_sort_by_x(tri);

    float dx = surface->dx;
    float dy = surface->dy;

    int xxi = idx_x(surface, tri.p1.x + dx / 2.0);
    int xxf = idx_x(surface, tri.p3.x - dx / 2.0);

    for (int xx = xxi; xx <= xxf; ++xx)
    {
        float x = (xx + 0.5) * dx;
        float y_1 = limit_y_1(&tri, x);
        float y_2 = limit_y_2(&tri, x);

        float yi = mini(y_1, y_2);
        float yf = maxi(y_1, y_2);

        int yyi = idx_y(surface, yi + dy / 2.0);
        int yyf = idx_y(surface, yf - dy / 2.0);

        for (int yy = yyi; yy <= yyf; ++yy)
        {
            struct pixel *pix = &surface->pixels[yy * surface->size_x + xx];

            float depth = triangle_depth(surface, &tri, normal, xx, yy);

            if (depth < pix->z)
            {
                pix->z = depth;
                pix->c = c;
            }
        }
    }
}

void surface_print(FILE *fp, const struct surface *surface)
{
    for (int yy = 0; yy < surface->size_y; ++yy)
    {
        for (int xx = 0; xx < surface->size_x; ++xx)
        {
            fprintf(fp, "%c", surface->pixels[yy * surface->size_x + xx].c);
        }
        fprintf(fp, "\n");
    }
}

void surface_printw(const struct surface *surface)
{
    for (int yy = 0; yy < surface->size_y; ++yy)
    {
        move(yy, 0);
        for (int xx = 0; xx < surface->size_x; ++xx)
        {
            printw("%c", surface->pixels[yy * surface->size_x + xx].c);
        }
    }
}
