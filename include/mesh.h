#pragma once

#include "mesh.h"

#include <SDL3/SDL_gpu.h>

typedef struct mesh_t mesh_t;

mesh_t *gpu_mesh_create(SDL_GPUDevice *device, mesh_info_t info);

void gpu_mesh_destroy(mesh_t *mesh);

void gpu_mesh_draw(const mesh_t *mesh, SDL_GPURenderPass *render_pass);
