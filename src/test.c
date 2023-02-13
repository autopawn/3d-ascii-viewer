#include "renderer.h"

#include <stdlib.h>
#include <unistd.h>
#include <time.h>

vec3 point_update(vec3 *point, vec3 *speed)
{
    point->x += speed->x;
    point->y += speed->y;
    point->z += speed->z;

    if (point->x < 0)
    {
        point->x = 0;
        speed->x *= -1;
    }
    if (point->x > 1.0)
    {
        point->x = 1;
        speed->x *= -1;
    }
    if (point->y < 0)
    {
        point->y = 0;
        speed->y *= -1;
    }
    if (point->y > 1.0)
    {
        point->y = 1;
        speed->y *= -1;
    }
    if (point->z < 0)
    {
        point->z = 0;
        speed->z *= -1;
    }
    if (point->z > 1.0)
    {
        point->z = 1;
        speed->z *= -1;
    }
}

int main()
{
    srand(time(NULL));

    vec3 p1 = (vec3) {0.5, 0.5, 0.5};
    vec3 p2 = (vec3) {0.5, 0.5, 0.5};
    vec3 p3 = (vec3) {0.5, 0.5, 0.5};

    vec3 p1_speed = (vec3) {rand()%5/100.0, rand()%5/100.0, rand()%5/100.0};
    vec3 p2_speed = (vec3) {rand()%5/100.0, rand()%5/100.0, rand()%5/100.0};
    vec3 p3_speed = (vec3) {rand()%5/100.0, rand()%5/100.0, rand()%5/100.0};

    for (float i = 0; i < 200; i++)
    {
        point_update(&p1, &p1_speed);
        point_update(&p2, &p2_speed);
        point_update(&p3, &p3_speed);

        struct surface *surface = surface_init(30, 20);

        struct triangle tri1 = {0};
        tri1.p1 = p1;
        tri1.p2 = p2;
        tri1.p3 = p3;
        surface_draw_triangle(surface, tri1);

        struct triangle tri2 = {0};
        tri2.p1 = p1;
        tri2.p2 = p3;
        tri2.p3 = p2;
        surface_draw_triangle(surface, tri2);

        surface_print(stdout, surface);

        surface_free(surface);

        usleep(100000);
    }
}
