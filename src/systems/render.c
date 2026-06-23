#include "camera.h"
#include "ecs.h"
#include "logcategory.h"
#include "math.h"
#include "model.h"
#include "systems.h"

#include "dcimgui.h"
#include "backends/dcimgui_impl_sdlgpu3.h"

#include "flecs.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

static void begin_render(ecs_iter_t *iter)
{
	SDL_Window *window = *ecs_field(iter, window_t*, 0);
	SDL_GPUDevice *device = *ecs_field(iter, gpu_device_t*, 1);
	SDL_FColor clear_color = *ecs_field(iter, clear_color_t, 2);
	SDL_GPUTexture *depth_texture = *ecs_field(iter, depth_texture_t*, 3);
	ImDrawData *imgui_draw_data = ecs_field(iter, imgui_draw_data_t, 4);
	SDL_GPUGraphicsPipeline *pipeline = *ecs_field(iter, gpu_graphics_pipeline_t*, 5);
	SDL_GPUCommandBuffer **command_buffer = ecs_field(iter, gpu_command_buffer_t*, 6);
	SDL_GPURenderPass **render_pass = ecs_field(iter, gpu_render_pass_t*, 7);
	SDL_GPUTexture **swapchain_texture = ecs_field(iter, swapchain_texture_t*, 8);
	vector2f_t *swapchain_texture_size = ecs_field(iter, swapchain_texture_size_t, 9);

	*command_buffer = SDL_AcquireGPUCommandBuffer(device);
	if (*command_buffer == nullptr)
	{
		SDL_LogWarn(LOG_CATEGORY_RENDER, "Failed to acquire command buffer: %s", SDL_GetError());
		return;
	}

	Uint32 swapchain_texture_width = 0;
	Uint32 swapchain_texture_height = 0;

	if (!SDL_WaitAndAcquireGPUSwapchainTexture(*command_buffer, window,
		swapchain_texture, &swapchain_texture_width, &swapchain_texture_height))
	{
		SDL_LogWarn(LOG_CATEGORY_RENDER, "Failed to acquire swapchain texture: %s", SDL_GetError());
		SDL_CancelGPUCommandBuffer(*command_buffer);
		return;
	}

	swapchain_texture_size->x = (float) swapchain_texture_width;
	swapchain_texture_size->y = (float) swapchain_texture_height;

	if (*swapchain_texture == nullptr)
	{
		return;
	}

	if (imgui_draw_data != nullptr) // TODO: Move to own system
	{
		cImGui_ImplSDLGPU3_PrepareDrawData(imgui_draw_data, *command_buffer);
	}

	const SDL_GPUColorTargetInfo color_target_info = {
		.texture = *swapchain_texture,
		.clear_color = clear_color,
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE,
	};

	const SDL_GPUDepthStencilTargetInfo depth_stencil_target_info = {
		.texture = depth_texture,
		.cycle = false,
		.clear_depth = 1,
		.clear_stencil = 0,
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE,
		.stencil_load_op = SDL_GPU_LOADOP_CLEAR,
		.stencil_store_op = SDL_GPU_STOREOP_STORE,
	};

	*render_pass = SDL_BeginGPURenderPass(*command_buffer, &color_target_info, 1,
		depth_texture != nullptr ? &depth_stencil_target_info : nullptr);

	SDL_BindGPUGraphicsPipeline(*render_pass, pipeline);
}

static void rebuild_camera_projection(ecs_iter_t *iter)
{
	const camera_t *camera = ecs_field(iter, camera_t, 0);
	const vector2f_t *size = ecs_field(iter, swapchain_texture_size_t, 1);
	matrix4x4_t *view_proj = ecs_field(iter, view_projection_t, 2);

	const float aspect = size != nullptr
		? size->x / size->y
		: 1280.F / 720.F; // Fallback to 16:9

	const matrix4x4_t proj = matrix4x4_create_perspective(deg2rad(camera->fov_y),
		aspect, camera->near_plane, camera->far_plane);

	const matrix4x4_t view = matrix4x4_create_look_at(camera->position,
		camera->target, camera->up);

	*view_proj = matrix4x4_multiply(view, proj);
}

static void render_scene(ecs_iter_t *iter)
{
	SDL_assert(iter->count == 1);

	SDL_GPURenderPass *render_pass = *ecs_field(iter, gpu_render_pass_t*, 0);
	SDL_GPUCommandBuffer *command_buffer = *ecs_field(iter, gpu_command_buffer_t*, 1);
	const matrix4x4_t view_proj = *ecs_field(iter, view_projection_t, 2);
	const model_t *model = ecs_field(iter, model_t, 3);

	model_draw(model, render_pass, command_buffer, view_proj);
}

static void rebuild_model_projection(projection_t *projection, const vector3f_t *scale,
	const vector3f_t *rotation, const vector3f_t *position)
{
	Uint8 mul = 0;
	projection->value = matrix4x4_zero();

	if (scale != nullptr)
	{
		projection->value = mul++ > 0
			? matrix4x4_multiply(projection->value, matrix4x4_create_scale(*scale))
			: matrix4x4_create_scale(*scale);
	}

	if (rotation != nullptr)
	{
		const matrix4x4_t transform = matrix4x4_multiply_n((matrix4x4_t[]){
			matrix4x4_create_rotation_x(rotation->x),
			matrix4x4_create_rotation_y(rotation->y),
			matrix4x4_create_rotation_z(rotation->z),
		}, 3);

		projection->value = mul++ > 0
			? matrix4x4_multiply(projection->value, transform)
			: transform;
	}

	if (position != nullptr)
	{
		projection->value = mul > 0
			? matrix4x4_multiply(projection->value, matrix4x4_create_translation(*position))
			: matrix4x4_create_translation(*position);
	}

	projection->rebuild = false;
}

static void render_model(ecs_iter_t *iter)
{
	SDL_assert(iter->count == 1); // TODO

	SDL_GPURenderPass *render_pass = *ecs_field(iter, gpu_render_pass_t*, 0);
	SDL_GPUCommandBuffer *command_buffer = *ecs_field(iter, gpu_command_buffer_t*, 1);
	const matrix4x4_t view_proj = *ecs_field(iter, view_projection_t, 2);

	const ecs_id_t pair_id = ecs_field_id(iter, 3);
	projection_t *projection = ecs_field(iter, projection_t, 4);

	const ecs_id_t model_id = ecs_lookup(ecs_world(), "chirp.Model");
	const ecs_entity_t model_entity = ecs_pair_second(ecs_world(), pair_id);
	const model_t *model = ecs_get_id(ecs_world(), model_entity, model_id);
	const size_t index = *ecs_field(iter, size_t, 3);

	// TODO: Maybe do this in pre-render?
	if (projection->rebuild)
	{
		const vector3f_t *scale = ecs_field(iter, scale_t, 5);
		const vector3f_t *rotation = ecs_field(iter, rotation_t, 6);
		const vector3f_t *position = ecs_field(iter, position_t, 7);
		rebuild_model_projection(projection, scale, rotation, position);
	}

	model_draw_indexed(model, index, render_pass, command_buffer,
		matrix4x4_multiply(projection->value, view_proj));
}

static void end_render(ecs_iter_t *iter)
{
	SDL_GPURenderPass *render_pass = *ecs_field(iter, gpu_render_pass_t*, 0);
	SDL_GPUCommandBuffer *command_buffer = *ecs_field(iter, gpu_command_buffer_t*, 1);
	SDL_GPUTexture *swapchain_texture = *ecs_field(iter, swapchain_texture_t*, 2);

	if (command_buffer == nullptr)
	{
		ecs_set_error("Render error", "Can't end rendering before starting it");
		return;
	}

	if (swapchain_texture != nullptr)
	{
		SDL_EndGPURenderPass(render_pass);
	}

	if (!SDL_SubmitGPUCommandBuffer(command_buffer))
	{
		ecs_set_error("Render error", SDL_GetError());
	}
}

void system_register_render()
{
	const ecs_entity_t engine = ecs_lookup(ecs_world(), "chirp.Engine");
	ecs_add_id(ecs_world(), engine, ecs_lookup(ecs_world(), "chirp.GpuCommandBuffer"));
	ecs_add_id(ecs_world(), engine, ecs_lookup(ecs_world(), "chirp.GpuRenderPass"));
	ecs_add_id(ecs_world(), engine, ecs_lookup(ecs_world(), "chirp.SwapchainTexture"));
	ecs_add_id(ecs_world(), engine, ecs_lookup(ecs_world(), "chirp.SwapchainTextureSize"));
	ecs_add_id(ecs_world(), engine, ecs_lookup(ecs_world(), "chirp.ViewProjection"));

	ecs_system_init(ecs_world(), &(ecs_system_desc_t){
		.entity = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
			.name = "BeginRender",
			.add = ecs_ids(ecs_dependson(ecs_phase(PHASE_RENDER_BEGIN))),
		}),
		.query.expr =
		"[in] chirp.Window, [in] chirp.GpuDevice, [in] chirp.ClearColor, [in] ?chirp.DepthTexture,"
		"[in] ?chirp.ImGuiDrawData, [in] chirp.GpuGraphicsPipeline, [out] chirp.GpuCommandBuffer,"
		"[out] chirp.GpuRenderPass, [out] chirp.SwapchainTexture, [out] chirp.SwapchainTextureSize",
		.callback = begin_render,
	});

	ecs_system_init(ecs_world(), &(ecs_system_desc_t){
		.entity = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
			.name = "RebuildCameraProjection",
			.add = ecs_ids(ecs_dependson(ecs_phase(PHASE_RENDER_BEGIN))),
		}),
		.query.expr = "[in] chirp.Camera, [in] ?chirp.SwapchainTextureSize, [out] chirp.ViewProjection",
		.callback = rebuild_camera_projection,
	});

	ecs_system_init(ecs_world(), &(ecs_system_desc_t){
		.entity = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
			.name = "RenderScene",
			.add = ecs_ids(ecs_dependson(ecs_phase(PHASE_RENDER))),
		}),
		.query.expr =
		"[in] chirp.GpuRenderPass, [in] chirp.GpuCommandBuffer,"
		"[in] chirp.ViewProjection, [in] chirp.Model, [none] chirp.Scene",
		.callback = render_scene,
	});

	ecs_system_init(ecs_world(), &(ecs_system_desc_t){
		.entity = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
			.name = "RenderModel",
			.add = ecs_ids(ecs_dependson(ecs_phase(PHASE_RENDER))),
		}),
		.query.expr =
		"[in] chirp.GpuRenderPass, [in] chirp.GpuCommandBuffer, [in] chirp.ViewProjection,"
		"[in] (chirp.InstanceOf, *), [inout] chirp.Projection, [in] ?chirp.Scale,"
		"[in] ?chirp.Rotation, [in] ?chirp.Position",
		.callback = render_model,
	});

	ecs_system_init(ecs_world(), &(ecs_system_desc_t){
		.entity = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
			.name = "EndRender",
			.add = ecs_ids(ecs_dependson(ecs_phase(PHASE_RENDER_END))),
		}),
		.query.expr = "[in] chirp.GpuRenderPass, [in] chirp.GpuCommandBuffer, [in] ?chirp.SwapchainTexture",
		.callback = end_render,
	});
}
