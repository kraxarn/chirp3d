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

bool gpu_upload_mesh_info(SDL_GPUDevice *device, const mesh_info_t info,
	SDL_GPUBuffer **vertex_buffer, SDL_GPUBuffer **index_buffer)
{
	const size_t vertex_size = sizeof(vertex_t) * info.num_vertices;
	const size_t index_size = sizeof(mesh_index_t) * info.num_indices;

	const SDL_GPUBufferCreateInfo vertex_buffer_info = {
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = vertex_size,
	};
	*vertex_buffer = SDL_CreateGPUBuffer(device, &vertex_buffer_info);
	if (*vertex_buffer == nullptr)
	{
		return false;
	}

	const SDL_GPUBufferCreateInfo index_buffer_info = {
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = index_size,
	};
	*index_buffer = SDL_CreateGPUBuffer(device, &index_buffer_info);
	if (*index_buffer == nullptr)
	{
		SDL_ReleaseGPUBuffer(device, *vertex_buffer);
		return false;
	}

	const SDL_GPUTransferBufferCreateInfo transfer_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = vertex_size + index_size,
	};
	SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
	if (transfer_buffer == nullptr)
	{
		SDL_ReleaseGPUBuffer(device, *vertex_buffer);
		SDL_ReleaseGPUBuffer(device, *index_buffer);
		return false;
	}

	void *transfer_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
	if (transfer_data == nullptr)
	{
		SDL_ReleaseGPUBuffer(device, *vertex_buffer);
		SDL_ReleaseGPUBuffer(device, *index_buffer);
		SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
		return false;
	}

	SDL_memcpy(transfer_data, info.vertices, vertex_size);
	SDL_memcpy(transfer_data + vertex_size, info.indices, index_size);

	SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

	SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(device);
	if (command_buffer == nullptr)
	{
		SDL_ReleaseGPUBuffer(device, *vertex_buffer);
		SDL_ReleaseGPUBuffer(device, *index_buffer);
		SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
		return false;
	}

	SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

	const SDL_GPUTransferBufferLocation vertex_source = {
		.transfer_buffer = transfer_buffer,
		.offset = 0,
	};
	const SDL_GPUBufferRegion vertex_destination = {
		.buffer = *vertex_buffer,
		.offset = 0,
		.size = vertex_size,
	};
	SDL_UploadToGPUBuffer(copy_pass, &vertex_source, &vertex_destination, false);

	const SDL_GPUTransferBufferLocation index_source = {
		.transfer_buffer = transfer_buffer,
		.offset = vertex_size,
	};
	const SDL_GPUBufferRegion index_destination = {
		.buffer = *index_buffer,
		.offset = 0,
		.size = index_size,
	};
	SDL_UploadToGPUBuffer(copy_pass, &index_source, &index_destination, false);

	SDL_EndGPUCopyPass(copy_pass);
	SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

	if (!SDL_SubmitGPUCommandBuffer(command_buffer))
	{
		SDL_ReleaseGPUBuffer(device, *vertex_buffer);
		SDL_ReleaseGPUBuffer(device, *index_buffer);
		return false;
	}

	return true;
}

bool gpu_upload_texture(SDL_GPUDevice *device, const SDL_Surface *surface,
	const SDL_GPUSamplerCreateInfo *sampler_info,
	SDL_GPUSampler **sampler, SDL_GPUTexture **texture)
{
	*sampler = SDL_CreateGPUSampler(device, sampler_info);
	if (*sampler == nullptr)
	{
		return false;
	}

	const SDL_GPUTextureCreateInfo texture_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.width = surface->w,
		.height = surface->h,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
	};
	*texture = SDL_CreateGPUTexture(device, &texture_info);
	if (*texture == nullptr)
	{
		return false;
	}

	const SDL_GPUTransferBufferCreateInfo buffer_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = surface->w * surface->h * 4,
	};
	SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(device, &buffer_info);
	if (transfer_buffer == nullptr)
	{
		return false;
	}

	void *transfer_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
	if (transfer_data == nullptr)
	{
		SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
		return false;
	}

	SDL_memcpy(transfer_data, surface->pixels, (size_t) (surface->w * surface->h * 4));

	SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

	SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(device);
	if (command_buffer == nullptr)
	{
		return false;
	}

	SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

	const SDL_GPUTextureTransferInfo source = {
		.transfer_buffer = transfer_buffer,
		.offset = 0,
	};
	const SDL_GPUTextureRegion destination = {
		.texture = *texture,
		.w = surface->w,
		.h = surface->h,
		.d = 1,
	};
	SDL_UploadToGPUTexture(copy_pass, &source, &destination, false);

	SDL_EndGPUCopyPass(copy_pass);

	if (!SDL_SubmitGPUCommandBuffer(command_buffer))
	{
		SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
		return false;
	}

	SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

	return true;
}
