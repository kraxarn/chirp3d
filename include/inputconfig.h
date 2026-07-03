#pragma once

#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>

typedef struct input_config_t
{
	SDL_Keycode *keycodes;
	SDL_MouseButtonFlags mouse_button;
} input_config_t;
