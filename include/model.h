#pragma once

#include "mesh.h"
#include "vector.h"

#include <SDL3/SDL_gpu.h>

[[nodiscard]]
mesh_t *create_cube(SDL_GPUDevice *device, vector3f_t position, vector3f_t size);
