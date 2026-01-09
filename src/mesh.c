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
	vector3f_t position;
} mesh_t;

mesh_t *mesh_create(SDL_GPUDevice *device, const mesh_info_t info)
{
	mesh_t *mesh = SDL_malloc(sizeof(mesh_t));

	mesh->device = device;
	mesh->num_indices = info.num_indices;
	mesh->texture = nullptr;
	mesh->sampler = nullptr;
	mesh->rebuild_projection = true;
	mesh->rotation = vector3f_zero();
	mesh->position = vector3f_zero();

	if (!gpu_upload_mesh_info(device, info, &mesh->vertex_buffer, &mesh->index_buffer))
	{
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
	// TODO: This isn't ideal :/ (but saves some confusion for now)
	if (texture != nullptr && texture->format != SDL_PIXELFORMAT_ABGR8888)
	{
		return SDL_SetError("Only ABGR8888 pixel format is currently supported");
	}

	if (mesh->texture != nullptr)
	{
		SDL_ReleaseGPUTexture(mesh->device, mesh->texture);
		SDL_ReleaseGPUSampler(mesh->device, mesh->sampler);
		mesh->texture = nullptr;
	}

	if (texture == nullptr)
	{
		return SDL_SetError("No texture provided");
	}

	const SDL_GPUSamplerCreateInfo sampler_info = {
		.min_filter = SDL_GPU_FILTER_NEAREST,
		.mag_filter = SDL_GPU_FILTER_NEAREST,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
	};

	return gpu_upload_texture(mesh->device, texture, &sampler_info,
		&mesh->sampler, &mesh->texture);
}

static void rebuild_projection(mesh_t *mesh)
{
	const matrix4x4_t transforms[] = {
		matrix4x4_create_translation(mesh->position),
		matrix4x4_create_rotation_x(deg2rad(mesh->rotation.x)),
		matrix4x4_create_rotation_y(deg2rad(mesh->rotation.y)),
		matrix4x4_create_rotation_z(deg2rad(mesh->rotation.z)),
	};

	mesh->projection = transforms[0];
	for (auto i = 1; i < SDL_arraysize(transforms); i++)
	{
		mesh->projection = matrix4x4_multiply(mesh->projection, transforms[i]);
	}

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

vector3f_t mesh_position(const mesh_t *mesh)
{
	return mesh->position;
}

void mesh_set_position(mesh_t *mesh, const vector3f_t position)
{
	mesh->position = position;
	mesh->rebuild_projection = true;
}
