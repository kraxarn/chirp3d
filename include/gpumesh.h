#pragma once

#include "mesh.h"

#include <SDL3/SDL_gpu.h>

typedef struct gpu_mesh_t gpu_mesh_t;

gpu_mesh_t *gpu_mesh_create(SDL_GPUDevice *device, mesh_t mesh);

void gpu_mesh_destroy(gpu_mesh_t *mesh);

void gpu_mesh_draw(const gpu_mesh_t *mesh, SDL_GPURenderPass *render_pass);
