#include "gpu.h"
#include "mesh.h"

#include <SDL3/SDL_gpu.h>
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
			.has_depth_stencil_target = true,                        // ?
			.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM, // ?
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
				// Colour
				(SDL_GPUVertexAttribute){
					.location = 1,
					.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
					.offset = sizeof(float) * 3,
				},
			},
		},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_shader = vertex_shader,
		.fragment_shader = fragment_shader,
		.depth_stencil_state = (SDL_GPUDepthStencilState){
			// ?
			.enable_depth_test = true,
			.enable_depth_write = true,
			.compare_op = SDL_GPU_COMPAREOP_LESS,
		},
	};

	return SDL_CreateGPUGraphicsPipeline(device, &create_info);
}

bool create_mesh_buffers(SDL_GPUDevice *device, const mesh_info_t mesh,
	SDL_GPUBuffer **vertex_buffer, SDL_GPUBuffer **index_buffer)
{
	const SDL_GPUBufferCreateInfo vertex_buffer_info = {
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = mesh_vertex_size(mesh),
	};
	*vertex_buffer = SDL_CreateGPUBuffer(device, &vertex_buffer_info);
	if (*vertex_buffer == nullptr)
	{
		return false;
	}

	const SDL_GPUBufferCreateInfo index_buffer_info = {
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = mesh_index_size(mesh),
	};
	*index_buffer = SDL_CreateGPUBuffer(device, &index_buffer_info);
	if (*index_buffer == nullptr)
	{
		SDL_ReleaseGPUBuffer(device, *vertex_buffer);
		return false;
	}

	return true;
}
