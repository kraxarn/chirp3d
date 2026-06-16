#pragma once

#include "physics.h"

#include <SDL3/SDL_stdinc.h>

typedef struct
{
	Uint16 fps;
	Uint16 count;
	float duration;
} time_info_t;

typedef struct
{
	Uint64 last_update;
	time_info_t time;
	float dt;

	physics_body_id_t player_body_id;
} app_state_t;
