#pragma once

#include "trigonometry.h"

// Get the center of the bounding box that contains the points.
vec3 get_bounding_box_center(const vec3 *A, int n);

// Get the maximum distance between the points in A and p
float get_max_dist(const vec3 *A, int n, vec3 p);
