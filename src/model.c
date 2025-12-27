#include "model.h"
#include "mesh.h"
#include "meshinfo.h"
#include "vector.h"

#include <SDL3/SDL_gpu.h>

#if 0
static void compute_normals(vertex_t *vertices, const size_t num_vertices,
	const mesh_index_t *indices, const size_t num_indicies)
{
	for (size_t i = 0; i < num_vertices; i++)
	{
		vertices[i].normal = vector3f_zero();
	}

	for (size_t i = 0; i < num_indicies; i += 3)
	{
		const mesh_index_t mi0 = indices[i];
		const mesh_index_t mi1 = indices[i + 1];
		const mesh_index_t mi2 = indices[i + 2];

		const vertex_t mv0 = vertices[mi0];
		const vertex_t mv1 = vertices[mi1];
		const vertex_t mv2 = vertices[mi2];

		const vector3f_t me1 = vector3f_sub(mv1.position, mv0.position);
		const vector3f_t me2 = vector3f_sub(mv2.position, mv0.position);
		const vector3f_t normal = vector3f_normalize(vector3f_cross(me1, me2));

		vertices[mi0].normal = vector3f_add(vertices[mi0].normal, normal);
		vertices[mi1].normal = vector3f_add(vertices[mi1].normal, normal);
		vertices[mi2].normal = vector3f_add(vertices[mi2].normal, normal);
	}

	for (size_t i = 0; i < num_vertices; i++)
	{
		vertices[i].normal = vector3f_normalize(vertices[i].normal);
	}
}
#endif

mesh_t *create_cube(SDL_GPUDevice *device, const vector3f_t position, const vector3f_t size)
{
	const vector3f_t min = {
		.x = position.x - (size.x / 2.F),
		.y = position.y - (size.y / 2.F),
		.z = position.z - (size.z / 2.F),
	};

	const vector3f_t max = {
		.x = position.x + (size.x / 2.F),
		.y = position.y + (size.y / 2.F),
		.z = position.z + (size.z / 2.F),
	};

	vertex_t vertices[] = {
		(vertex_t){
			.position = (vector3f_t){.x = min.x, .y = min.y, .z = max.z},
			.color = (SDL_FColor){.r = 1.F, .g = 0.F, .b = 0.F, .a = 1.F},
		},
		(vertex_t){
			.position = (vector3f_t){.x = max.x, .y = min.y, .z = max.z},
			.color = (SDL_FColor){.r = 0.F, .g = 1.F, .b = 0.F, .a = 1.F},
		},
		(vertex_t){
			.position = (vector3f_t){.x = max.x, .y = max.y, .z = max.z},
			.color = (SDL_FColor){.r = 0.F, .g = 0.F, .b = 1.F, .a = 1.F},
		},
		(vertex_t){
			.position = (vector3f_t){.x = min.x, .y = max.y, .z = max.z},
			.color = (SDL_FColor){.r = 1.F, .g = 1.F, .b = 0.F, .a = 1.F},
		},
		(vertex_t){
			.position = (vector3f_t){.x = min.x, .y = min.y, .z = min.z},
			.color = (SDL_FColor){.r = 0.F, .g = 1.F, .b = 1.F, .a = 1.F},
		},
		(vertex_t){
			.position = (vector3f_t){.x = max.x, .y = min.y, .z = min.z},
			.color = (SDL_FColor){.r = 1.F, .g = 0.F, .b = 1.F, .a = 1.F},
		},
		(vertex_t){
			.position = (vector3f_t){.x = max.x, .y = max.y, .z = min.z},
			.color = (SDL_FColor){.r = 1.F, .g = 1.F, .b = 1.F, .a = 1.F},
		},
		(vertex_t){
			.position = (vector3f_t){.x = min.x, .y = max.y, .z = min.z},
			.color = (SDL_FColor){.r = 0.F, .g = 0.F, .b = 0.F, .a = 1.F},
		},
	};

	const mesh_index_t indices[] = {
		0, 1, 2,
		0, 2, 3,
		1, 5, 6,
		1, 6, 2,
		4, 6, 5,
		4, 7, 6,
		4, 0, 3,
		4, 3, 7,
		3, 2, 6,
		3, 6, 7,
		0, 4, 5,
		0, 5, 1,
	};

	const mesh_info_t mesh_info = {
		.num_vertices = SDL_arraysize(vertices),
		.vertices = vertices,
		.num_indices = SDL_arraysize(indices),
		.indices = indices,
	};

	return mesh_create(device, mesh_info);
}
