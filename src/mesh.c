#include "mesh.h"
#include "gpu.h"
#include "meshinfo.h"

#include <SDL3/SDL_stdinc.h>

typedef struct mesh_t
{
	SDL_GPUDevice *device;
	SDL_GPUBuffer *vertex_buffer;
	SDL_GPUBuffer *index_buffer;

	size_t num_indices;
} mesh_t;

mesh_t *gpu_mesh_create(SDL_GPUDevice *device, const mesh_info_t info)
{
	mesh_t *mesh = SDL_malloc(sizeof(mesh_t));

	mesh->device = device;
	mesh->num_indices = info.num_indices;

	const SDL_GPUBufferCreateInfo vertex_buffer_info = {
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = mesh_vertex_size(info),
	};
	mesh->vertex_buffer = SDL_CreateGPUBuffer(device, &vertex_buffer_info);
	if (mesh->vertex_buffer == nullptr)
	{
		SDL_free(mesh);
		return nullptr;
	}

	const SDL_GPUBufferCreateInfo index_buffer_info = {
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = mesh_index_size(info),
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
		.size = mesh_upload_size(info),
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

	SDL_memcpy(transfer_data, info.vertices, mesh_vertex_size(info));
	SDL_memcpy(transfer_data + mesh_vertex_size(info), info.indices, mesh_index_size(info));

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
		.size = mesh_vertex_size(info),
	};
	SDL_UploadToGPUBuffer(copy_pass, &vertex_source, &vertex_destination, false);

	const SDL_GPUTransferBufferLocation index_source = {
		.transfer_buffer = transfer_buffer,
		.offset = mesh_vertex_size(info),
	};
	const SDL_GPUBufferRegion index_destination = {
		.buffer = mesh->index_buffer,
		.offset = 0,
		.size = mesh_index_size(info),
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

void gpu_mesh_destroy(mesh_t *mesh)
{
	SDL_ReleaseGPUBuffer(mesh->device, mesh->vertex_buffer);
	SDL_ReleaseGPUBuffer(mesh->device, mesh->index_buffer);
	SDL_free(mesh);
}

void gpu_mesh_draw(const mesh_t *mesh, SDL_GPURenderPass *render_pass)
{
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

	SDL_DrawGPUIndexedPrimitives(render_pass, mesh->num_indices, 1, 0, 0, 0);
}
