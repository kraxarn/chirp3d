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

typedef struct material_t material_t;
typedef struct node_t node_t;
typedef struct scene_camera_t scene_camera_t;

typedef struct model_t
{
	SDL_GPUDevice *device;

	material_t *materials;
	size_t material_count;

	node_t *nodes;
	size_t node_count;

	scene_camera_t *cameras;
	size_t camera_count;

	SDL_GPUSampler *sampler;
	SDL_GPUTexture *texture;
} model_t;

bool model_create(SDL_GPUDevice *device, SDL_IOStream *stream, bool close_io, model_t *model);

void model_destroy(const model_t *model);

void model_draw(const model_t *model, SDL_GPURenderPass *render_pass,
	SDL_GPUCommandBuffer *command_buffer, matrix4x4_t projection);

void model_draw_indexed(const model_t *model, size_t index,
	SDL_GPURenderPass *render_pass, SDL_GPUCommandBuffer *command_buffer,
	matrix4x4_t projection);

[[nodiscard]]
const char *model_node_name(const model_t *model, size_t index);

[[nodiscard]]
vector3f_t model_node_translation(const model_t *model, size_t index);
