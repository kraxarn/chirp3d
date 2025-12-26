#pragma once

#include "matrix.h"
#include "pointlight.h"
#include "vector.h"

#include <SDL3/SDL_pixels.h>

typedef struct vertex_uniform_data_t
{
	matrix4x4_t mvp;
	SDL_FColor color;
	vector4f_t camera_position;
	point_light_t lights[2];
} vertex_uniform_data_t;
