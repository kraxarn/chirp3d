#include "windowconfig.h"
#include "vector.h"

window_config_t window_config_default()
{
	constexpr auto width = 640;
	constexpr auto height = 480;

	return (window_config_t){
		.title = "game window",
		.size = (vector2i_t){
			.x = width,
			.y = height,
		},
		.fullscreen = false,
	};
}

SDL_WindowFlags window_config_flags(const window_config_t window_config)
{
	return (int) window_config.fullscreen
		? SDL_WINDOW_FULLSCREEN
		: SDL_WINDOW_RESIZABLE;
}
