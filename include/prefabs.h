#pragma once

#include "ecs.h"

#define prefab_model(n)			\
	EcsModelInstance,			\
	sizeof(model_instance_t),	\
	&(model_instance_t){.name = SDL_strdup(n) /* TODO: Memory leak */}
