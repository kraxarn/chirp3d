#pragma once

#include "matrix.h"
#include "vector.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>

typedef Uint16 mesh_index_t;

typedef struct vertex_t
{
	vector3f_t position;
	vector3f_t normal;
	vector2f_t tex_coord;
	vector4f_t color;
} vertex_t;

typedef struct model_t model_t;

[[nodiscard]]
model_t *model_create(SDL_GPUDevice *device, SDL_IOStream *stream, bool close_io);

void model_destroy(model_t *model);

void model_draw(const model_t *model, SDL_GPURenderPass *render_pass,
	SDL_GPUCommandBuffer *command_buffer, matrix4x4_t projection);

void model_set_rotation(const model_t *model, vector3f_t rotation);

[[nodiscard]]
vector3f_t model_node_position(const model_t *model, const char *node);

void model_set_position(const model_t *model, vector3f_t position);

void model_set_scale(const model_t *model, vector3f_t scale);
