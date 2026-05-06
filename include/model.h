#pragma once

#include "matrix.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_iostream.h>

typedef struct model_t model_t;

[[nodiscard]]
model_t *model_create(SDL_GPUDevice *device, SDL_IOStream *stream, bool close_io);

void model_destroy(model_t *model);

void model_draw(const model_t *model, SDL_GPURenderPass *render_pass,
	SDL_GPUCommandBuffer *command_buffer, matrix4x4_t projection);
