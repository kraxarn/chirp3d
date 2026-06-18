#pragma once

#include <SDL3/SDL_stdinc.h>

typedef struct
{
	Uint64 last_update;
	Uint16 fps;
	Uint16 count;
	float duration;
	float dt;
} app_state_t;
