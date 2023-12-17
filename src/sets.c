#include "sets.h"
#include "trigonometry.h"

vec3 get_bounding_box_center(const vec3 *A, int n)
{
    vec3 min = {0};
    vec3 max = {0};

    if (n > 0)
    {
        min = A[0];
        max = A[0];
    }

    for (int i = 0; i < n; i++)
    {
        vec3 v = A[i];

        if (v.x < min.x)
            min.x = v.x;
        if (v.y < min.y)
            min.y = v.y;
        if (v.z < min.z)
            min.z = v.z;

        if (v.x > max.x)
            max.x = v.x;
        if (v.y > max.y)
            max.y = v.y;
        if (v.z > max.z)
            max.z = v.z;
    }

    vec3 center;
    center.x = (min.x + max.x)/2.0;
    center.y = (min.y + max.y)/2.0;
    center.z = (min.z + max.z)/2.0;

    return center;
}

float get_max_dist(const vec3 *A, int n, vec3 p)
{
    float max_d2 = 0.0;

    for (int i = 0; i < n; ++i)
    {
        vec3 v = A[i];
        float d2 = (v.x - p.x) * (v.x - p.x) + (v.y - p.y) * (v.y - p.y) + (v.z - p.z) * (v.z - p.z);

        if (max_d2 < d2)
            max_d2 = d2;
    }

    return sqrtf(max_d2);
}
