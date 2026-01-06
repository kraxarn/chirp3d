#pragma once

#include "inputconfig.h"

#include <SDL3/SDL_events.h>

void input_update(const SDL_Event *event);

bool input_add(const char *name, input_config_t config);

[[nodiscard]]
bool input_is_key_down(SDL_Keycode keycode);

[[nodiscard]]
bool input_is_down(const char *name);
