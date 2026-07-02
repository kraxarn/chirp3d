#pragma once

#include "ecs.h"
#include "inputconfig.h"

#include <SDL3/SDL_events.h>

typedef struct
{
	ecs_entity_t parent; // TODO: Remove from here
} input_t;

bool input_create(input_t *input);

void input_update(const SDL_Event *event);

bool input_add(const input_t *input, const char *name, input_config_t config);

[[nodiscard]]
bool input_is_pressed(const char *name);

[[nodiscard]]
bool input_is_down(const char *name);
