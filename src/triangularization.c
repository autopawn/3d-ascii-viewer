#include "triangularization.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static float absfloat(float a)
{
    return (a >= 0) ? a : -a;
}

static float triangle_area(vec3 p1, vec3 p2, vec3 p3)
{
    return absfloat(p1.x * (p2.y - p3.y) + p2.x * (p3.y - p1.y) + p3.x * (p1.y - p2.y))/2.0;
}

static bool point_in_triangle(vec3 pt, vec3 v1, vec3 v2, vec3 v3)
{
    float atot = triangle_area(v1, v2, v3);
    float a1 = triangle_area(v1, v2, pt);
    float a2 = triangle_area(v2, v3, pt);
    float a3 = triangle_area(v3, v1, pt);

    return a1 + a2 + a3 <= atot * 1.00001;
}

static void triangularize_recurse(vec3 *vecs, int *idxs, int n, bool orient, int *out_idxs)
{
    assert(n >= 3);

    if (n == 3)
    {
        out_idxs[0] = idxs[0];
        out_idxs[1] = idxs[1];
        out_idxs[2] = idxs[2];
        return;
    }

    // Find convex angle
    int i1, i2, i3;
    vec3 v1, v2, v3;
    for (int t = 0; t < n; ++t)
    {
        i1 = (n / 2 + t + (n - 1)) % n;
        i2 = (n / 2 + t) % n;
        i3 = (n / 2 + t + 1) % n;

        v1 = vecs[i1];
        v2 = vecs[i2];
        v3 = vecs[i3];

        vec3 d1 = vec3_sub(v3, v2);
        vec3 d2 = vec3_sub(v1, v2);
        float cross_prod = vec3_cross_product(d1, d2).z;
        bool convex = cross_prod == 0 || ((cross_prod > 0) != orient);

        if (convex)
            break;
    }

    // Rect equation ax + by + c = 0 for the line between v1 and v3
    float a = v1.y - v3.y;
    float b = v3.x - v1.x;
    float c = (v1.x - v3.x) * v1.y + (v3.y - v1.y) * v1.x;

    // Find point inside the (v1,v2,v3) triangle with largest perpendicular distance to line (v1,v3)
    int max_dist_k = -1;
    float max_dist = 0;
    for (int k = 0; k < n; k++)
    {
        if (k == i1 || k == i2 || k == i3)
            continue;

        if (point_in_triangle(vecs[k], v1, v2, v3))
        {
            // Perpendicular distance (multiplied by sqrt(a^2 + b^2))
            float dist = absfloat(a * vecs[k].x + b * vecs[k].y + c);

            if (max_dist_k == -1 || dist > max_dist)
            {
                max_dist = dist;
                max_dist_k = k;
            }
        }
    }

    if (max_dist_k == -1)
    {
        // Cut this ear on i2.
        out_idxs[0] = idxs[i1];
        out_idxs[1] = idxs[i2];
        out_idxs[2] = idxs[i3];

        int n2 = 0;
        for (int r = 0; r < n; ++r)
        {
            if (r == i2)
                continue;
            idxs[n2] = idxs[r];
            vecs[n2] = vecs[r];
            ++n2;
        }
        assert(n2 == n - 1);
        triangularize_recurse(vecs, idxs, n2, orient, out_idxs + 3);
    }
    else
    {
        // Create a diagonal from i2 to max_dist_k, split the problem in two.
        int n1 = 0;
        int n2 = 0;
        vec3 *vecs1 = NULL;
        vec3 *vecs2 = NULL;
        int *idxs1 = NULL;
        int *idxs2 = NULL;

        vecs1 = malloc(n * sizeof(vec3));
        vecs2 = malloc(n * sizeof(vec3));
        idxs1 = malloc(n * sizeof(int));
        idxs2 = malloc(n * sizeof(int));

        if (!vecs1 || !vecs2 || !idxs1 || !idxs2)
        {
            fprintf(stderr, "ERROR: Memory allocation failure.\n");
            exit(1);
        }

        bool side = false;
        for (int r = 0; r < n; ++r)
        {
            if (r == i2 || r == max_dist_k)
            {
                vecs1[n1] = vecs[r];
                idxs1[n1] = idxs[r];
                ++n1;
                vecs2[n2] = vecs[r];
                idxs2[n2] = idxs[r];
                ++n2;
                side = !side;
            }
            else if (side)
            {
                vecs1[n1] = vecs[r];
                idxs1[n1] = idxs[r];
                ++n1;
            }
            else
            {
                vecs2[n2] = vecs[r];
                idxs2[n2] = idxs[r];
                ++n2;
            }
        }

        assert(n1 + n2 == n + 2);
        triangularize_recurse(vecs1, idxs1, n1, orient, out_idxs);
        triangularize_recurse(vecs2, idxs2, n2, orient, out_idxs + 3 * (n1 - 2));

        free(vecs1);
        free(vecs2);
        free(idxs1);
        free(idxs2);
    }
}

void triangularize(const vec3 *vecs, int n, int *out_idxs)
{
    assert(n >= 3);

    // Find the plane that contains all vectors (given by <dir1,dir2>)
    float best_normal_mag = 0;
    vec3 dir1, dir2;

    for (int i = 0; i < n; ++i)
    {
        vec3 v1 = vecs[i];
        vec3 v2 = vecs[(i + 1) % n];
        vec3 v3 = vecs[(i + 2) % n];

        vec3 d1 = vec3_sub(v1, v2);
        vec3 d2 = vec3_sub(v3, v2);
        vec3 normal = vec3_cross_product(d1, d2);

        if (best_normal_mag <= vec3_mag(normal))
        {
            dir1 = vec3_normalize(d1);
            dir2 = vec3_cross_product(vec3_normalize(normal), d1);
        }
    }

    // Translate all vectors to plane coordinates
    vec3 *vecs_plane = malloc(n * sizeof(vec3));
    if (!vecs_plane)
    {
        fprintf(stderr, "ERROR: Memory allocation failure.\n");
        exit(1);
    }

    for (int i = 0; i < n; ++i)
    {
        vecs_plane[i].x = vec3_dot_product(dir1, vecs[i]);
        vecs_plane[i].y = vec3_dot_product(dir2, vecs[i]);
        vecs_plane[i].z = 0;
    }

    // Find orientation
    float area = 0;
    for (int i = 0; i < n; ++i)
    {
        vec3 v1 = vecs_plane[i];
        vec3 v2 = vecs_plane[(i + 1) % n];

        area += (v2.x - v1.x) * (v2.y + v1.y);
    }
    bool orientation = area >= 0;

    // Vector indexes
    int *idxs = malloc(n * sizeof(int));
    if (!idxs)
    {
        fprintf(stderr, "ERROR: Memory allocation failure.\n");
        exit(1);
    }
    for (int i = 0; i < n; ++i)
        idxs[i] = i;

    triangularize_recurse(vecs_plane, idxs, n, orientation, out_idxs);
    free(vecs_plane);
    free(idxs);

    return;
}
