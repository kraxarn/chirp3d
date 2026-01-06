#pragma once

#include "matrix.h"
#include "meshinfo.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_surface.h>

typedef struct mesh_t mesh_t;

[[nodiscard]]
mesh_t *mesh_create(SDL_GPUDevice *device, mesh_info_t info);

void mesh_destroy(mesh_t *mesh);

bool mesh_set_texture(mesh_t *mesh, const SDL_Surface *texture);

void mesh_draw(const mesh_t *mesh, SDL_GPURenderPass *render_pass,
	SDL_GPUCommandBuffer *command_buffer, matrix4x4_t projection);

[[nodiscard]]
vector3f_t mesh_rotation(const mesh_t *mesh);

void mesh_set_rotation(mesh_t *mesh, vector3f_t rotation);
