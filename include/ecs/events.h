#pragma once

#include <SDL3/SDL_stdinc.h>

typedef Uint64 ecs_id_t;
typedef Uint64 ecs_entity_t;

extern ecs_entity_t EcsOnMouseButton;
extern ecs_id_t EcsMouseButtonEvent;

extern ecs_entity_t EcsOnKey;
extern ecs_id_t EcsKeyboardEvent;

extern ecs_entity_t EcsOnWindowResized;
extern ecs_id_t EcsWindowEvent;
