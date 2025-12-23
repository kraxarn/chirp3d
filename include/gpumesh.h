#pragma once

#include "mesh.h"

#include <SDL3/SDL_gpu.h>

typedef struct gpu_mesh_t
{
	SDL_GPUDevice *device;
	SDL_GPUBuffer *vertex_buffer;
	SDL_GPUBuffer *index_buffer;

	size_t num_indices;
} gpu_mesh_t;

gpu_mesh_t *gpu_mesh_create(SDL_GPUDevice *device, mesh_t mesh);

void gpu_mesh_destroy(gpu_mesh_t *mesh);
