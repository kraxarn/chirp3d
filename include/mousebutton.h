#pragma once

#include <SDL3/SDL_mouse.h>

[[nodiscard]]
const char *mouse_button_name(SDL_MouseButtonFlags flags);

[[nodiscard]]
SDL_MouseButtonFlags mouse_button_from_name(const char *name);
