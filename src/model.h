#pragma once

#include "trigonometry.h"

#include <stdbool.h>

struct face
{
    unsigned int idxs[3];
};

struct model
{
    unsigned int vertex_count;
    unsigned int vertex_capacity;
    vec3 *vertexes;

    unsigned int faces_count;
    unsigned int faces_capacity;
    struct face *faces;
};

struct model *model_load_from_obj(const char *fname);

void model_bounding_box(const struct model *model, vec3 *minp, vec3 *maxp);

void model_invert_triangles(struct model *model);

// Scale the model so that it fits in the [-1, 1]^3 cube with any rotation.
void model_normalize(struct model *model, bool invert_z);

void model_free(struct model *model);
