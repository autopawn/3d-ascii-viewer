#pragma once

#include "trigonometry.h"

struct model
{
    unsigned int vertex_count;
    unsigned int vertex_capacity;
    vec3 *vertexes;

    unsigned int faces_count;
    unsigned int faces_capacity;
    unsigned int *idxs;
};

struct model *model_init_from_obj(const char *fname);

void model_bounding_box(const struct model *model, vec3 *minp, vec3 *maxp);

void model_free(struct model *model);
