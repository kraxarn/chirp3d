#pragma once

#include "meshinfo.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_surface.h>

typedef struct mesh_t mesh_t;

[[nodiscard]]
mesh_t *mesh_create(SDL_GPUDevice *device, mesh_info_t info);

void mesh_destroy(mesh_t *mesh);

bool mesh_set_texture(mesh_t *mesh, const SDL_Surface *texture);

void mesh_draw(const mesh_t *mesh, SDL_GPURenderPass *render_pass);
