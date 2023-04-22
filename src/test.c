#include "renderer.h"
#include "model.h"

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

struct transform
{
    vec3 offset;
    float zoom;
};

struct transform transform_from_model(const struct model *model)
{
    struct transform t = {0};
    vec3 min, max;

    model_bounding_box(model, &min, &max);

    t.zoom = 1.0 / sqrtf((min.x - max.x)*(min.x - max.x) + (min.y - max.y)*(min.y - max.y)
            + (min.z - max.z)*(min.z - max.z));
    t.offset.x = - (min.x + max.x) / 2.0;
    t.offset.y = - (min.y + max.y) / 2.0;
    t.offset.z = - (min.z + max.z) / 2.0;

    return t;
}

vec3 apply_transform(const struct transform *t, vec3 v)
{
    v.x += t->offset.x;
    v.y += t->offset.y;
    v.z += t->offset.z;
    v.x *= t->zoom;
    v.y *= -t->zoom;
    v.z *= t->zoom;

    // center for screen
    v.x += 0.5;
    v.y += 0.5;
    v.z += 0.5;

    return v;
}

vec3 vec3_rotate_y(float cos, float sin, vec3 v)
{
    float x = v.x * cos - v.z * sin;
    float z = v.x * sin + v.z * cos;
    v.x = x;
    v.z = z;
    return v;
}

vec3 vec3_rotate_x(float cos, float sin, vec3 v)
{
    float y = v.y * cos - v.z * sin;
    float z = v.y * sin + v.z * cos;
    v.y = y;
    v.z = z;
    return v;
}

int main(int argc, char const *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <input>.obj\n", argv[0]);
        return 1;
    }

    struct surface *surface = surface_init(160, 100);

    struct model *model = model_init_from_obj(argv[1]);
    if (!model)
        return 1;

    struct transform tf = transform_from_model(model);

    if (!surface)
        return 1;

    for (int t = 0; t < 200; t++)
    {
        float elev = 0.125 * 3.14159265358979323846 * (sinf(0.05 * t) - 1);
        float elev_cos = cosf(elev);
        float elev_sin = sinf(elev);

        float az = 0.12 * t;
        float az_cos = cosf(az);
        float az_sin = sinf(az);

        surface_clear(surface);

        for (int f = 0; f < model->faces_count; ++f)
        {
            int i1 = model->idxs[3 * f + 0];
            int i2 = model->idxs[3 * f + 1];
            int i3 = model->idxs[3 * f + 2];

            vec3 v1 = model->vertexes[i1];
            vec3 v2 = model->vertexes[i2];
            vec3 v3 = model->vertexes[i3];

            v1 = vec3_rotate_y(az_cos, az_sin, v1);
            v2 = vec3_rotate_y(az_cos, az_sin, v2);
            v3 = vec3_rotate_y(az_cos, az_sin, v3);

            v1 = vec3_rotate_x(elev_cos, elev_sin, v1);
            v2 = vec3_rotate_x(elev_cos, elev_sin, v2);
            v3 = vec3_rotate_x(elev_cos, elev_sin, v3);

            struct triangle tri = {0};
            tri.p1 = apply_transform(&tf, v1);
            tri.p2 = apply_transform(&tf, v2);
            tri.p3 = apply_transform(&tf, v3);

            surface_draw_triangle(surface, tri);
        }

        surface_print(stdout, surface);
        usleep(100000);
    }

    surface_free(surface);
    model_free(model);
}
