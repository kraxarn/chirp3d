#pragma once

#include "map.h"
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
typedef struct camera_t camera_t;

typedef struct model_t
{
	SDL_GPUDevice *device;

	material_t *materials;
	size_t material_count;

	node_t *nodes;
	size_t node_count;

	map_t node_indices;

	camera_t *cameras;
	size_t camera_count;

	SDL_GPUSampler *sampler;
	SDL_GPUTexture *texture;
} model_t;

typedef struct
{
	const model_t *model;
	const node_t *node;

	vector3f_t rotation;
	vector3f_t position;
	vector3f_t scale;

	matrix4x4_t projection;
	bool rebuild_projection;
} node_instance_t;

bool model_create(SDL_GPUDevice *device, SDL_IOStream *stream, bool close_io, model_t *model);

bool model_create_instance(const model_t *model, const char *name, node_instance_t *instance);

void model_destroy(const model_t *model);

void model_draw(const model_t *model, SDL_GPURenderPass *render_pass,
	SDL_GPUCommandBuffer *command_buffer, matrix4x4_t projection);

void model_instance_draw(node_instance_t *instance, SDL_GPURenderPass *render_pass,
	SDL_GPUCommandBuffer *command_buffer, matrix4x4_t projection);

[[nodiscard]]
vector3f_t model_node_position(const model_t *model, const char *node);

void model_instance_set_rotation(node_instance_t *instance, vector3f_t rotation);

[[nodiscard]]
vector3f_t model_instance_position(const node_instance_t *instance);

void model_instance_set_position(node_instance_t *instance, vector3f_t position);

void model_instance_set_scale(node_instance_t *instance, vector3f_t scale);
