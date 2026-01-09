#include "gpu.h"
#include "logcategory.h"
#include "meshinfo.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

static constexpr SDL_GPUShaderFormat shader_formats =
	SDL_GPU_SHADERFORMAT_SPIRV
	| SDL_GPU_SHADERFORMAT_DXIL
	| SDL_GPU_SHADERFORMAT_MSL;

static constexpr auto debug_mode =
#ifdef NDEBUG
	false;
#else
	true;
#endif

static SDL_GPUCommandBuffer *current_command_buffer = nullptr;
static SDL_GPURenderPass *current_render_pass = nullptr;

static SDL_GPUTexture *swapchain_texture = nullptr;
static Uint32 swapchain_texture_width = 0;
static Uint32 swapchain_texture_height = 0;

SDL_GPUDevice *create_device(SDL_Window *window)
{
	SDL_GPUDevice *device = SDL_CreateGPUDevice(shader_formats, debug_mode, nullptr);
	if (device == nullptr)
	{
		return nullptr;
	}

	if (!SDL_ClaimWindowForGPUDevice(device, window))
	{
		SDL_DestroyGPUDevice(device);
		return nullptr;
	}

	return device;
}

SDL_GPUGraphicsPipeline *create_pipeline(SDL_GPUDevice *device, SDL_Window *window,
	SDL_GPUShader *vertex_shader, SDL_GPUShader *fragment_shader)
{
	const SDL_GPUGraphicsPipelineCreateInfo create_info = {
		.target_info = (SDL_GPUGraphicsPipelineTargetInfo){
			.num_color_targets = 1,
			.color_target_descriptions = (SDL_GPUColorTargetDescription[]){
				(SDL_GPUColorTargetDescription){
					.format = SDL_GetGPUSwapchainTextureFormat(device, window),
					.blend_state = (SDL_GPUColorTargetBlendState){
						.enable_blend = true,
						.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
						.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
						.color_blend_op = SDL_GPU_BLENDOP_ADD,
						.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
						.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
						.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
					},
				}
			},
			.has_depth_stencil_target = true,
			.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
		},
		.depth_stencil_state = (SDL_GPUDepthStencilState){
			.enable_depth_test = true,
			.enable_depth_write = true,
			.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL,
		},
		.vertex_input_state = (SDL_GPUVertexInputState){
			.num_vertex_buffers = 1,
			.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){
				(SDL_GPUVertexBufferDescription){
					.slot = 0,
					.pitch = sizeof(vertex_t),
					.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
				},
			},
			.num_vertex_attributes = 2,
			.vertex_attributes = (SDL_GPUVertexAttribute[]){
				// Position
				(SDL_GPUVertexAttribute){
					.location = 0,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
					.offset = 0,
				},
				// Texture coordinate
				(SDL_GPUVertexAttribute){
					.location = 1,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
					.offset = sizeof(vector3f_t),
				},
			},
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.rasterizer_state = (SDL_GPURasterizerState){
			.cull_mode = SDL_GPU_CULLMODE_BACK,
		},
		.vertex_shader = vertex_shader,
		.fragment_shader = fragment_shader,
	};

	return SDL_CreateGPUGraphicsPipeline(device, &create_info);
}

SDL_GPUTexture *create_depth_texture(SDL_GPUDevice *device, const vector2i_t size)
{
	const SDL_GPUTextureCreateInfo create_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.width = size.x,
		.height = size.y,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = SDL_GPU_SAMPLECOUNT_1,
		.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET
	};

	return SDL_CreateGPUTexture(device, &create_info);
}

char *gpu_driver_names()
{
	// "vulkan, metal, direct3d12"
	constexpr size_t gpu_drivers_len = 25;

	char *gpu_drivers = SDL_malloc(sizeof(char) * (gpu_drivers_len + 1));
	if (gpu_drivers == nullptr)
	{
		return nullptr;
	}
	gpu_drivers[0] = '\0';

	for (auto i = 0; i < SDL_GetNumGPUDrivers(); i++)
	{
		SDL_strlcat(gpu_drivers, SDL_GetGPUDriver(i), gpu_drivers_len);
		if (i < SDL_GetNumGPUDrivers() - 1)
		{
			SDL_strlcat(gpu_drivers, ", ", gpu_drivers_len);
		}
	}

	gpu_drivers[SDL_strlen(gpu_drivers)] = '\0';
	return gpu_drivers;
}

bool draw_begin(SDL_GPUDevice *device, SDL_Window *window, const SDL_FColor clear_color,
	SDL_GPUTexture *depth_texture, SDL_GPUCommandBuffer **command_buffer,
	SDL_GPURenderPass **render_pass, vector2f_t *size)
{
	current_command_buffer = SDL_AcquireGPUCommandBuffer(device);
	if (current_command_buffer == nullptr)
	{
		SDL_LogWarn(LOG_CATEGORY_RENDER, "Failed to acquire command buffer: %s", SDL_GetError());
		return false;
	}
	*command_buffer = current_command_buffer;

	if (!SDL_WaitAndAcquireGPUSwapchainTexture(*command_buffer, window,
		&swapchain_texture, &swapchain_texture_width, &swapchain_texture_height))
	{
		SDL_LogWarn(LOG_CATEGORY_RENDER, "Failed to acquire swapchain texture: %s", SDL_GetError());
		SDL_CancelGPUCommandBuffer(*command_buffer);
		return false;
	}

	size->x = (float) swapchain_texture_width;
	size->y = (float) swapchain_texture_height;

	if (swapchain_texture == nullptr)
	{
		return false;
	}

	const SDL_GPUColorTargetInfo color_target_info = {
		.texture = swapchain_texture,
		.clear_color = clear_color,
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE,
	};
	const SDL_GPUDepthStencilTargetInfo depth_stencil_target_info = {
		.texture = depth_texture,
		.cycle = true,
		.clear_depth = 1,
		.clear_stencil = 0,
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE,
		.stencil_load_op = SDL_GPU_LOADOP_CLEAR,
		.stencil_store_op = SDL_GPU_STOREOP_STORE,
	};
	current_render_pass = SDL_BeginGPURenderPass(*command_buffer, &color_target_info, 1,
		&depth_stencil_target_info);
	*render_pass = current_render_pass;
	return true;
}

bool draw_end()
{
	if (swapchain_texture != nullptr)
	{
		SDL_EndGPURenderPass(current_render_pass);
	}

	return SDL_SubmitGPUCommandBuffer(current_command_buffer);
}
