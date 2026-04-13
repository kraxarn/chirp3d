#pragma once

#include <SDL3/SDL_iostream.h>

typedef struct model_t model_t;

model_t *model_create(SDL_IOStream *stream, bool close_io);

void model_destroy(model_t *model);
