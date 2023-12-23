#pragma once

#include "trigonometry.h"

#include <stdbool.h>

#define MATERIAL_NAME_BUFFER_SIZE 256

struct face
{
    unsigned int idxs[3];
    int material; // -1 means no material.
};

struct material
{
    char name[MATERIAL_NAME_BUFFER_SIZE];
    float Kd_r, Kd_g, Kd_b;
};

struct model
{
    unsigned int vertex_count;
    unsigned int vertex_capacity;
    vec3 *vertexes;

    unsigned int faces_count;
    unsigned int faces_capacity;
    struct face *faces;

    unsigned int materials_count;
    unsigned int materials_capacity;
    struct material *materials;
};

struct model *model_load_from_obj(const char *fname, bool color_support);
struct model *model_load_from_stl(const char *fname);

void model_invert_triangles(struct model *model);

// Scale the model so that it fits in the [-1, 1]^3 cube with any rotation.
void model_normalize(struct model *model);

void model_change_orientation(struct model *model, int axis1, int axis2, int axis3);

void model_invert_x(struct model *model);
void model_invert_y(struct model *model);
void model_invert_z(struct model *model);

void model_free(struct model *model);
