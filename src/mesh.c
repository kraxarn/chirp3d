#include "mesh.h"
#include "gpu.h"
#include "math.h"
#include "matrix.h"
#include "meshinfo.h"
#include "uniformdata.h"
#include "vector.h"

#include <SDL3/SDL_stdinc.h>

typedef struct mesh_t
{
	SDL_GPUDevice *device;
	SDL_GPUBuffer *vertex_buffer;
	SDL_GPUBuffer *index_buffer;
	SDL_GPUTexture *texture;
	SDL_GPUSampler *sampler;

	size_t num_indices;

	matrix4x4_t projection;
	bool rebuild_projection;

	vector3f_t rotation;
} mesh_t;

mesh_t *mesh_create(SDL_GPUDevice *device, const mesh_info_t info)
{
	mesh_t *mesh = SDL_malloc(sizeof(mesh_t));

	mesh->device = device;
	mesh->num_indices = info.num_indices;
	mesh->texture = nullptr;
	mesh->sampler = nullptr;
	mesh->rotation = vector3f_zero();
	mesh->rebuild_projection = true;

	const size_t vertex_size = sizeof(vertex_t) * info.num_vertices;
	const size_t index_size = sizeof(mesh_index_t) * info.num_indices;

	const SDL_GPUBufferCreateInfo vertex_buffer_info = {
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = vertex_size,
	};
	mesh->vertex_buffer = SDL_CreateGPUBuffer(device, &vertex_buffer_info);
	if (mesh->vertex_buffer == nullptr)
	{
		SDL_free(mesh);
		return nullptr;
	}

	const SDL_GPUBufferCreateInfo index_buffer_info = {
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = index_size,
	};
	mesh->index_buffer = SDL_CreateGPUBuffer(device, &index_buffer_info);
	if (mesh->index_buffer == nullptr)
	{
		SDL_ReleaseGPUBuffer(device, mesh->vertex_buffer);
		SDL_free(mesh);
		return nullptr;
	}

	const SDL_GPUTransferBufferCreateInfo transfer_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = vertex_size + index_size,
	};
	SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
	if (transfer_buffer == nullptr)
	{
		SDL_ReleaseGPUBuffer(device, mesh->vertex_buffer);
		SDL_ReleaseGPUBuffer(device, mesh->index_buffer);
		SDL_free(mesh);
		return nullptr;
	}

	void *transfer_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
	if (transfer_data == nullptr)
	{
		SDL_ReleaseGPUBuffer(device, mesh->vertex_buffer);
		SDL_ReleaseGPUBuffer(device, mesh->index_buffer);
		SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
		SDL_free(mesh);
		return nullptr;
	}

	SDL_memcpy(transfer_data, info.vertices, vertex_size);
	SDL_memcpy(transfer_data + vertex_size, info.indices, index_size);

	SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

	SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(device);
	if (command_buffer == nullptr)
	{
		SDL_ReleaseGPUBuffer(device, mesh->vertex_buffer);
		SDL_ReleaseGPUBuffer(device, mesh->index_buffer);
		SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
		SDL_free(mesh);
		return nullptr;
	}

	SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

	const SDL_GPUTransferBufferLocation vertex_source = {
		.transfer_buffer = transfer_buffer,
		.offset = 0,
	};
	const SDL_GPUBufferRegion vertex_destination = {
		.buffer = mesh->vertex_buffer,
		.offset = 0,
		.size = vertex_size,
	};
	SDL_UploadToGPUBuffer(copy_pass, &vertex_source, &vertex_destination, false);

	const SDL_GPUTransferBufferLocation index_source = {
		.transfer_buffer = transfer_buffer,
		.offset = vertex_size,
	};
	const SDL_GPUBufferRegion index_destination = {
		.buffer = mesh->index_buffer,
		.offset = 0,
		.size = index_size,
	};
	SDL_UploadToGPUBuffer(copy_pass, &index_source, &index_destination, false);

	SDL_EndGPUCopyPass(copy_pass);
	SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

	if (!SDL_SubmitGPUCommandBuffer(command_buffer))
	{
		SDL_ReleaseGPUBuffer(device, mesh->vertex_buffer);
		SDL_ReleaseGPUBuffer(device, mesh->index_buffer);
		SDL_free(mesh);
		return nullptr;
	}

	return mesh;
}

void mesh_destroy(mesh_t *mesh)
{
	if (mesh == nullptr)
	{
		return;
	}

	if (mesh->texture != nullptr)
	{
		SDL_ReleaseGPUTexture(mesh->device, mesh->texture);
		SDL_ReleaseGPUSampler(mesh->device, mesh->sampler);
	}

	SDL_ReleaseGPUBuffer(mesh->device, mesh->vertex_buffer);
	SDL_ReleaseGPUBuffer(mesh->device, mesh->index_buffer);
	SDL_free(mesh);
}

bool mesh_set_texture(mesh_t *mesh, const SDL_Surface *texture)
{
	if (mesh->texture != nullptr)
	{
		SDL_ReleaseGPUTexture(mesh->device, mesh->texture);
		SDL_ReleaseGPUSampler(mesh->device, mesh->sampler);
	}

	const SDL_GPUSamplerCreateInfo sampler_info = {
		.min_filter = SDL_GPU_FILTER_NEAREST,
		.mag_filter = SDL_GPU_FILTER_NEAREST,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
	};

	mesh->sampler = SDL_CreateGPUSampler(mesh->device, &sampler_info);
	if (mesh->sampler == nullptr)
	{
		return false;
	}

	const SDL_GPUTextureCreateInfo texture_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.width = texture->w,
		.height = texture->h,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
	};

	mesh->texture = SDL_CreateGPUTexture(mesh->device, &texture_info);
	if (mesh->texture == nullptr)
	{
		return false;
	}

	const SDL_GPUTransferBufferCreateInfo buffer_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = texture->w * texture->h * 4,
	};

	SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(mesh->device, &buffer_info);
	if (transfer_buffer == nullptr)
	{
		return false;
	}

	void *transfer_data = SDL_MapGPUTransferBuffer(mesh->device, transfer_buffer, false);
	if (transfer_data == nullptr)
	{
		SDL_ReleaseGPUTransferBuffer(mesh->device, transfer_buffer);
		return false;
	}

	SDL_memcpy(transfer_data, texture->pixels, (size_t) (texture->w * texture->h * 4));
	SDL_UnmapGPUTransferBuffer(mesh->device, transfer_buffer);

	SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(mesh->device);
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
		.texture = mesh->texture,
		.w = texture->w,
		.h = texture->h,
		.d = 1,
	};
	SDL_UploadToGPUTexture(copy_pass, &source, &destination, false);

	SDL_EndGPUCopyPass(copy_pass);
	if (!SDL_SubmitGPUCommandBuffer(command_buffer))
	{
		SDL_ReleaseGPUTransferBuffer(mesh->device, transfer_buffer);
		return false;
	}

	SDL_ReleaseGPUTransferBuffer(mesh->device, transfer_buffer);

	return true;
}

static void rebuild_projection(mesh_t *mesh)
{
	const matrix4x4_t rotation_x = matrix4x4_create_rotation_x(deg2rad(mesh->rotation.x));
	const matrix4x4_t rotation_y = matrix4x4_create_rotation_y(deg2rad(mesh->rotation.y));
	const matrix4x4_t rotation_z = matrix4x4_create_rotation_z(deg2rad(mesh->rotation.z));

	mesh->projection = matrix4x4_multiply(rotation_x, matrix4x4_multiply(rotation_y, rotation_z));
	mesh->rebuild_projection = false;
}

void mesh_draw(mesh_t *mesh, SDL_GPURenderPass *render_pass, SDL_GPUCommandBuffer *command_buffer,
	const matrix4x4_t projection)
{
	if (mesh->rebuild_projection)
	{
		rebuild_projection(mesh);
	}

	const SDL_GPUBufferBinding vertex_binding = {
		.buffer = mesh->vertex_buffer,
		.offset = 0,
	};
	SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);

	const SDL_GPUBufferBinding index_binding = {
		.buffer = mesh->index_buffer,
		.offset = 0,
	};
	SDL_BindGPUIndexBuffer(render_pass, &index_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

	if (mesh->texture != nullptr)
	{
		const SDL_GPUTextureSamplerBinding binding = {
			.texture = mesh->texture,
			.sampler = mesh->sampler,
		};
		SDL_BindGPUFragmentSamplers(render_pass, 0, &binding, 1);
	}

	const vertex_uniform_data_t vertex_data = {
		.mvp = matrix4x4_multiply(mesh->projection, projection),
	};
	SDL_PushGPUVertexUniformData(command_buffer, 0, &vertex_data, sizeof(vertex_uniform_data_t));

	SDL_DrawGPUIndexedPrimitives(render_pass, mesh->num_indices, 1, 0, 0, 0);
}

vector3f_t mesh_rotation(const mesh_t *mesh)
{
	return mesh->rotation;
}

void mesh_set_rotation(mesh_t *mesh, const vector3f_t rotation)
{
	mesh->rotation = rotation;
	mesh->rebuild_projection = true;
}
