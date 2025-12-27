#include "gpu.h"
#include "logcategory.h"
#include "meshinfo.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
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
				}
			},
			.has_depth_stencil_target = true,
			.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
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
			.fill_mode = SDL_GPU_FILLMODE_FILL,
			.cull_mode = SDL_GPU_CULLMODE_NONE,
		},
		.depth_stencil_state = (SDL_GPUDepthStencilState){
			.enable_depth_test = true,
			.enable_depth_write = true,
			.compare_op = SDL_GPU_COMPAREOP_LESS,
		},
		.vertex_shader = vertex_shader,
		.fragment_shader = fragment_shader,
	};

	return SDL_CreateGPUGraphicsPipeline(device, &create_info);
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

char *shader_format_names(SDL_GPUDevice *device)
{
	// "Invalid, Private, SPIR-V, DXBC, DXIL, MSL, metallib"
	constexpr size_t shader_formats_len = 51;

	char *shader_formats = SDL_malloc(sizeof(char) * (shader_formats_len + 1));
	if (shader_formats == nullptr)
	{
		return nullptr;
	}

	const SDL_GPUShaderFormat shader_format = SDL_GetGPUShaderFormats(device);

	if (shader_format == SDL_GPU_SHADERFORMAT_INVALID)
	{
		SDL_strlcat(shader_formats, "invalid, ", shader_formats_len);
	}
	if ((shader_format & SDL_GPU_SHADERFORMAT_PRIVATE) > 0)
	{
		SDL_strlcat(shader_formats, "private, ", shader_formats_len);
	}
	if ((shader_format & SDL_GPU_SHADERFORMAT_SPIRV) > 0)
	{
		SDL_strlcat(shader_formats, "spir-v, ", shader_formats_len);
	}
	if ((shader_format & SDL_GPU_SHADERFORMAT_DXBC) > 0)
	{
		SDL_strlcat(shader_formats, "dxbc, ", shader_formats_len);
	}
	if ((shader_format & SDL_GPU_SHADERFORMAT_DXIL) > 0)
	{
		SDL_strlcat(shader_formats, "dxil, ", shader_formats_len);
	}
	if ((shader_format & SDL_GPU_SHADERFORMAT_MSL) > 0)
	{
		SDL_strlcat(shader_formats, "msl, ", shader_formats_len);
	}
	if ((shader_format & SDL_GPU_SHADERFORMAT_METALLIB) > 0)
	{
		SDL_strlcat(shader_formats, "metallib, ", shader_formats_len);
	}

	shader_formats[SDL_strlen(shader_formats) - 2] = '\0';
	return shader_formats;
}

bool draw_begin(SDL_GPUDevice *device, SDL_Window *window, const SDL_FColor clear_color,
	SDL_GPUCommandBuffer **command_buffer, SDL_GPURenderPass **render_pass, vector2f_t *size)
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
	current_render_pass = SDL_BeginGPURenderPass(*command_buffer, &color_target_info, 1, nullptr);
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
