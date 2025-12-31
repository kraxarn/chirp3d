#pragma once

#include "matrix.h"
#include "vector.h"

typedef struct vertex_uniform_data_t
{
	matrix4x4_t mvp;
	vector2f_aligned_t tex_uv[4];
} vertex_uniform_data_t;
