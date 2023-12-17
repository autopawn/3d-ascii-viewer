#pragma once

#include "trigonometry.h"

// Triangularize the face, filling *out_idxs with (n-2)*3 indexes from 0 to n -1,
// in groups of 3, each group a triangle.
void triangularize(const vec3 *vecs, int n, int *out_idxs);
