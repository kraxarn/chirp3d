#pragma once

#include "flecs.h"

#include <SDL3/SDL_stdinc.h>

void physics_ctor(void *ptr, Sint32 count,
	const ecs_type_info_t *type_info);

void physics_dtor(void *ptr, Sint32 count,
	const ecs_type_info_t *type_info);
