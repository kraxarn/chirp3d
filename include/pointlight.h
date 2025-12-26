#pragma once

#include "vector.h"

#include <SDL3/SDL_pixels.h>

typedef struct point_light_t
{
	vector3f_t position;
	SDL_FColor color;
	SDL_FColor ambient;
} point_light_t;
