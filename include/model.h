#pragma once

#include "matrix.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_iostream.h>

typedef struct model_t model_t;

[[nodiscard]]
model_t *model_create(SDL_GPUDevice *device, SDL_IOStream *stream, bool close_io);

void model_destroy(model_t *model);

void model_draw(model_t *model, SDL_GPURenderPass *render_pass,
	SDL_GPUCommandBuffer *command_buffer, matrix4x4_t projection);

[[nodiscard]]
vector3f_t model_rotation(const model_t *model, size_t node);

void model_set_rotation(const model_t *model, size_t node, vector3f_t rotation);

[[nodiscard]]
vector3f_t model_position(const model_t *model, size_t node);

void model_set_position(const model_t *model, size_t node, vector3f_t position);

[[nodiscard]]
vector3f_t model_scale(const model_t *model, size_t node);

void model_set_scale(const model_t *model, size_t node, vector3f_t scale);
