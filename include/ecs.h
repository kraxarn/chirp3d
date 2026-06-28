#pragma once

#include "matrix.h"
#include "vector.h"

#include "dcimgui.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>

typedef struct ecs_world_t ecs_world_t;
typedef Uint64 ecs_id_t;
typedef Uint64 ecs_entity_t;

typedef SDL_InitFlags init_flags_t;
typedef SDL_Window window_t;
typedef SDL_GPUDevice gpu_device_t;
typedef SDL_GPUGraphicsPipeline gpu_graphics_pipeline_t;
typedef SDL_GPUCommandBuffer gpu_command_buffer_t;
typedef SDL_GPURenderPass gpu_render_pass_t;
typedef SDL_GPUTexture depth_texture_t;
typedef SDL_GPUTexture swapchain_texture_t;
typedef vector2f_t swapchain_texture_size_t;
typedef size_t instance_of_index_t;
typedef vector3f_t rotation_t;
typedef vector3f_t position_t;
typedef vector3f_t scale_t;
typedef ImGuiContext imgui_context_t;
typedef ImDrawData imgui_draw_data_t;
typedef SDL_GPUShader vertex_shader_t;
typedef SDL_GPUShader fragment_shader_t;
typedef SDL_FColor clear_color_t;
typedef matrix4x4_t view_projection_t;
typedef Sint32 py_vm_index_t;

typedef struct
{
	bool rebuild;
	matrix4x4_t value;
} projection_t;

typedef struct
{
	char *title;
	char *message;
} error_t;

typedef enum : Uint8
{
	PHASE_UPDATE,         // Update game logic
	PHASE_PHYSICS_UPDATE, // Update/tick physics engine
	PHASE_PHYSICS_SYNC,   // Sync physics engine changes to entities
	PHASE_RENDER_BEGIN,   // Begin rendering
	PHASE_RENDER,         // Render all renderable entities
	PHASE_RENDER_END,     // End rendering and submit to GPU
	PHASE_COUNT,
} phase_t;

void ecs_create();

void ecs_destroy();

[[nodiscard]]
ecs_world_t *ecs_world();

[[nodiscard]]
ecs_entity_t ecs_phase(phase_t phase);

ecs_entity_t ecs_set_error(const char *title, const char *message);

[[nodiscard]]
void *ecs_get_id_ptr(ecs_id_t component);

#define ecs_observer_init_all(o)					\
	do {											\
	for (size_t i = 0; i < SDL_arraysize(o); i++)	\
		ecs_observer_init(ecs_world(), o + i);		\
	} while (false)

void ecs_add_assets();
void ecs_add_imgui();
void ecs_add_window();
void ecs_add_gpu();
void ecs_add_physics();
void ecs_add_render();
void ecs_add_input();
void ecs_add_script_engine();
