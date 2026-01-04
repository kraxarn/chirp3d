#pragma once

#include <SDL3/SDL_events.h>

void input_update(const SDL_Event *event);

[[nodiscard]]
bool input_is_key_down(SDL_Keycode keycode);
