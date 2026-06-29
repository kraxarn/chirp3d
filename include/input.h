#pragma once

#include "ecs.h"
#include "inputconfig.h"
#include "map.h"

#include <SDL3/SDL_events.h>

typedef struct
{
	map_t key_map;    // key name -> input state
	map_t button_map; // mouse button name -> input state
	map_t name_map;   // input name -> input map

	ecs_entity_t entity; // TODO: This should be the only thing needed
} input_t;

bool input_create(input_t *input);

void input_update(const input_t *input, const SDL_Event *event);

bool input_add(const input_t *input, const char *name, input_config_t config);

[[nodiscard]]
bool input_is_pressed(const input_t *input, const char *name);

[[nodiscard]]
bool input_is_down(const input_t *input, const char *name);
