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
typedef struct node_instance_t node_instance_t;

[[nodiscard]]
model_t *model_create(SDL_GPUDevice *device, SDL_IOStream *stream, bool close_io);

[[nodiscard]]
node_instance_t *model_create_instance(const model_t *model, const char *name);

void model_destroy(model_t *model);

void model_destroy_instance(node_instance_t *instance);

void model_draw(const model_t *model, SDL_GPURenderPass *render_pass,
	SDL_GPUCommandBuffer *command_buffer, matrix4x4_t projection);

void model_instance_draw(node_instance_t *instance, SDL_GPURenderPass *render_pass,
	SDL_GPUCommandBuffer *command_buffer, matrix4x4_t projection);

void model_set_rotation(node_instance_t *instance, vector3f_t rotation);

[[nodiscard]]
vector3f_t model_node_position(const model_t *model, const char *node);

void model_set_position(node_instance_t *instance, vector3f_t position);

void model_set_scale(node_instance_t *instance, vector3f_t scale);
