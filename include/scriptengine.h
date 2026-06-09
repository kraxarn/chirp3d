#pragma once

#include "ecs.h"

#include <SDL3/SDL_iostream.h>

void script_engine_create(ecs_world_t *ecs_world);

bool script_engine_exec(const char *filename, SDL_IOStream *stream, bool close_io);

void script_engine_destroy();
