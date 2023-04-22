#include "renderer.h"

#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

static const char LUM_OPTIONS[] = ".,':;!+*=#$@";
static const int LUM_OPTIONS_COUNT = sizeof(LUM_OPTIONS) - 1;
static const vec3 LIGHT_ORIGIN = {-1.0, 1.0, 0.0};

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

static float vec3_mag(vec3 v)
{
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

static vec3 vec3_normalize(vec3 v)
{
    float mag = vec3_mag(v);
    v.x /= mag;
    v.y /= mag;
    v.z /= mag;
    return v;
}

static float cosine_similarity(vec3 a, vec3 b)
{
    return (a.x * b.x + a.y * b.y + a.z * b.z) / (vec3_mag(a) * vec3_mag(b));
}

static char color_from_normal(vec3 normal)
{
    float sim = cosine_similarity(normal, LIGHT_ORIGIN) * 0.5 + 0.5;
    unsigned int p = (unsigned int) roundf((LUM_OPTIONS_COUNT - 1) * sim);
    return LUM_OPTIONS[p];
}

static bool triangle_orientation(const struct triangle *tri)
{
    return (tri->p2.x - tri->p1.x) * (tri->p3.y - tri->p2.y)
            > (tri->p3.x - tri->p2.x) * (tri->p2.y - tri->p1.y);
}

static vec3 triangle_normal(const struct triangle *tri)
{
    vec3 v1, v2, normal;

    v1.x = tri->p2.x - tri->p1.x;
    v1.y = tri->p2.y - tri->p1.y;
    v1.z = tri->p2.z - tri->p1.z;

    v2.x = tri->p3.x - tri->p1.x;
    v2.y = tri->p3.y - tri->p1.y;
    v2.z = tri->p3.z - tri->p1.z;

    normal.x = v1.y * v2.z - v1.z * v2.y;
    normal.y = v1.z * v2.x - v1.x * v2.z;
    normal.z = v1.x * v2.y - v1.y * v2.x;

    return vec3_normalize(normal);
}

static struct triangle triangle_sort_by_x(struct triangle triangle)
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


struct surface *surface_init(unsigned int size_x, unsigned int size_y)
{
    struct surface *surface = malloc(sizeof(*surface));
    assert(surface);

    surface->size_x = size_x;
    surface->size_y = size_y;

    surface->pixels = malloc(size_y * size_x * sizeof(*surface->pixels));
    assert(surface->pixels);
    surface_clear(surface);

    return surface;
}

void surface_clear(struct surface *surface)
{
    assert(surface);
    assert(surface->pixels);

    for (int i = 0; i < surface->size_y * surface->size_x; ++i)
    {
        surface->pixels[i].color = ' ';
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
    float dx = 1.0 / surface->size_x;
    return maxi(0, mini(surface->size_x - 1, (int) floorf(x / dx)));
}

static inline int idx_y(const struct surface *surface, float y)
{
    float dy = 1.0 / surface->size_y;
    return maxi(0, mini(surface->size_y - 1, (int) floorf(y / dy)));
}

static inline float limit_y_1(const struct triangle *tri, float x)
{
    if (x <= tri->p1.x)
        return tri->p1.y;
    if (x >= tri->p3.x)
        return tri->p3.y;
    if (x <= tri->p2.x)
        return tri->p1.y + (tri->p2.y - tri->p1.y) * (x - tri->p1.x) / (tri->p2.x - tri->p1.x);
    return tri->p2.y + (tri->p3.y - tri->p2.y) * (x - tri->p2.x) / (tri->p3.x - tri->p2.x);
}

static inline float limit_y_2(const struct triangle *tri, float x)
{
    if (x <= tri->p1.x)
        return tri->p1.y;
    if (x >= tri->p3.x)
        return tri->p3.y;
    return tri->p1.y + (tri->p3.y - tri->p1.y) * (x - tri->p1.x) / (tri->p3.x - tri->p1.x);
}

static inline float triangle_depth(const struct surface *surface, const struct triangle *tri,
        vec3 normal, int xx, int yy)
{
    float dx = 1.0 / surface->size_x;
    float dy = 1.0 / surface->size_y;

    float x = (xx + 0.5) * dx;
    float y = (yy + 0.5) * dy;

    return tri->p1.z - (normal.x * (x - tri->p1.x) + normal.y * (y - tri->p1.y)) / normal.z;
}

void surface_draw_triangle(struct surface *surface, struct triangle tri)
{
    if (!triangle_orientation(&tri))
        return;

    vec3 normal = triangle_normal(&tri);

    tri = triangle_sort_by_x(tri);

    char color = tri.color;
    if (!color)
        color = color_from_normal(normal);

    float dx = 1.0 / surface->size_x;
    float dy = 1.0 / surface->size_y;

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
                pix->color = color;
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
            fprintf(fp, "%c", surface->pixels[yy * surface->size_x + xx].color);
        }
        fprintf(fp, "\n");
    }
}
