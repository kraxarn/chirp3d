#include "gpumesh.h"
#include "gpu.h"
#include "mesh.h"

#include <SDL3/SDL_stdinc.h>

gpu_mesh_t *gpu_mesh_create(SDL_GPUDevice *device, const mesh_t mesh)
{
	gpu_mesh_t *gpu_mesh = SDL_malloc(sizeof(gpu_mesh_t));

	gpu_mesh->device = device;
	gpu_mesh->num_indices = mesh.num_indices;

	const SDL_GPUBufferCreateInfo vertex_buffer_info = {
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = mesh_vertex_size(mesh),
	};
	gpu_mesh->vertex_buffer = SDL_CreateGPUBuffer(device, &vertex_buffer_info);
	if (gpu_mesh->vertex_buffer == nullptr)
	{
		SDL_free(gpu_mesh);
		return nullptr;
	}

	const SDL_GPUBufferCreateInfo index_buffer_info = {
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = mesh_index_size(mesh),
	};
	gpu_mesh->index_buffer = SDL_CreateGPUBuffer(device, &index_buffer_info);
	if (gpu_mesh->index_buffer == nullptr)
	{
		SDL_ReleaseGPUBuffer(device, gpu_mesh->vertex_buffer);
		SDL_free(gpu_mesh);
		return nullptr;
	}

	const SDL_GPUTransferBufferCreateInfo transfer_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = mesh_upload_size(mesh),
	};
	SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_info);
	if (transfer_buffer == nullptr)
	{
		SDL_ReleaseGPUBuffer(device, gpu_mesh->vertex_buffer);
		SDL_ReleaseGPUBuffer(device, gpu_mesh->index_buffer);
		SDL_free(gpu_mesh);
		return nullptr;
	}

	void *transfer_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
	if (transfer_data == nullptr)
	{
		SDL_ReleaseGPUBuffer(device, gpu_mesh->vertex_buffer);
		SDL_ReleaseGPUBuffer(device, gpu_mesh->index_buffer);
		SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
		SDL_free(gpu_mesh);
		return nullptr;
	}

	SDL_memcpy(transfer_data, mesh.vertices, mesh_vertex_size(mesh));
	SDL_memcpy(transfer_data + mesh_vertex_size(mesh), mesh.indices, mesh_index_size(mesh));

	SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

	SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(device);
	if (command_buffer == nullptr)
	{
		SDL_ReleaseGPUBuffer(device, gpu_mesh->vertex_buffer);
		SDL_ReleaseGPUBuffer(device, gpu_mesh->index_buffer);
		SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
		SDL_free(gpu_mesh);
		return nullptr;
	}

	SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

	const SDL_GPUTransferBufferLocation vertex_source = {
		.transfer_buffer = transfer_buffer,
		.offset = 0,
	};
	const SDL_GPUBufferRegion vertex_destination = {
		.buffer = gpu_mesh->vertex_buffer,
		.offset = 0,
		.size = mesh_vertex_size(mesh),
	};
	SDL_UploadToGPUBuffer(copy_pass, &vertex_source, &vertex_destination, false);

	const SDL_GPUTransferBufferLocation index_source = {
		.transfer_buffer = transfer_buffer,
		.offset = mesh_vertex_size(mesh),
	};
	const SDL_GPUBufferRegion index_destination = {
		.buffer = gpu_mesh->index_buffer,
		.offset = 0,
		.size = mesh_index_size(mesh),
	};
	SDL_UploadToGPUBuffer(copy_pass, &index_source, &index_destination, false);

	SDL_EndGPUCopyPass(copy_pass);
	SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

	if (!SDL_SubmitGPUCommandBuffer(command_buffer))
	{
		SDL_ReleaseGPUBuffer(device, gpu_mesh->vertex_buffer);
		SDL_ReleaseGPUBuffer(device, gpu_mesh->index_buffer);
		SDL_free(gpu_mesh);
		return nullptr;
	}

	return gpu_mesh;
}

void gpu_mesh_destroy(gpu_mesh_t *mesh)
{
	SDL_ReleaseGPUBuffer(mesh->device, mesh->vertex_buffer);
	SDL_ReleaseGPUBuffer(mesh->device, mesh->index_buffer);
	SDL_free(mesh);
}

void gpu_mesh_draw(const gpu_mesh_t *mesh, SDL_GPURenderPass *render_pass)
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
