#pragma once

#include "matrix.h"

#include <SDL3/SDL_pixels.h>

typedef struct vertex_uniform_data_t
{
	matrix4x4_t mvp;
	SDL_FColor color;
} vertex_uniform_data_t;
