#pragma once

#include "vector.h"

#include <SDL3/SDL_video.h>

typedef struct window_config_t
{
	const char *title;
	vector2i_t size;
	bool fullscreen;
} window_config_t;

[[nodiscard]]
window_config_t window_config_default();

[[nodiscard]]
SDL_WindowFlags window_config_flags(window_config_t window_config);
