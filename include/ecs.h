#pragma once

#include "matrix.h"
#include "vector.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>

typedef struct ecs_world_t ecs_world_t;

typedef SDL_InitFlags init_flags_t;
typedef SDL_Window window_t;
typedef SDL_GPUDevice gpu_device_t;
typedef SDL_GPUGraphicsPipeline gpu_graphics_pipeline_t;
typedef SDL_GPUTexture depth_texture_t;
typedef size_t instance_of_index_t;
typedef vector3f_t rotation_t;
typedef vector3f_t position_t;
typedef vector3f_t scale_t;

typedef struct
{
	bool rebuild;
	matrix4x4_t value;
} projection_t;

void ecs_create();

void ecs_destroy();

[[nodiscard]]
ecs_world_t *ecs_world();

[[nodiscard]]
const void *ecs_const_data(const char *name);

[[nodiscard]]
void *ecs_mut_data_ptr(const char *name);

[[nodiscard]]
void *ecs_mut_data(const char *name);
