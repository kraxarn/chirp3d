#include "model.h"
#include "mesh.h"
#include "vector.h"

mesh_info_t create_cube([[maybe_unused]] const vector3f_t position, [[maybe_unused]] const vector3f_t size)
{
	mesh_info_t mesh;

	const vertex_t vertices[] = {
		(vertex_t){
			.position = (vector3f_t){.x = 0.000000, .y = 0.000000, .z = 1.000000},
			.color = (SDL_FColor){.r = 1.F, .g = 0.F, .b = 0.F, .a = 1.F},
		},
		(vertex_t){
			.position = (vector3f_t){.x = 1.000000, .y = 0.000000, .z = 1.000000},
			.color = (SDL_FColor){.r = 0.F, .g = 1.F, .b = 0.F, .a = 1.F},
		},
		(vertex_t){
			.position = (vector3f_t){.x = 1.000000, .y = 1.000000, .z = 1.000000},
			.color = (SDL_FColor){.r = 0.F, .g = 0.F, .b = 1.F, .a = 1.F},
		},
		(vertex_t){
			.position = (vector3f_t){.x = 0.000000, .y = 1.000000, .z = 1.000000},
			.color = (SDL_FColor){.r = 1.F, .g = 1.F, .b = 0.F, .a = 1.F},
		},
		(vertex_t){
			.position = (vector3f_t){.x = 0.000000, .y = 0.000000, .z = 0.000000},
			.color = (SDL_FColor){.r = 0.F, .g = 1.F, .b = 1.F, .a = 1.F},
		},
		(vertex_t){
			.position = (vector3f_t){.x = 1.000000, .y = 0.000000, .z = 0.000000},
			.color = (SDL_FColor){.r = 1.F, .g = 0.F, .b = 1.F, .a = 1.F},
		},
		(vertex_t){
			.position = (vector3f_t){.x = 1.000000, .y = 1.000000, .z = 0.000000},
			.color = (SDL_FColor){.r = 1.F, .g = 1.F, .b = 1.F, .a = 1.F},
		},
		(vertex_t){
			.position = (vector3f_t){.x = 0.000000, .y = 1.000000, .z = 0.000000},
			.color = (SDL_FColor){.r = 0.F, .g = 0.F, .b = 0.F, .a = 1.F},
		},
	};

	mesh.num_vertices = SDL_arraysize(vertices);
	mesh.vertices = SDL_malloc(sizeof(vertex_t) * mesh.num_vertices);
	SDL_memcpy(mesh.vertices, vertices, sizeof(vertex_t) * mesh.num_vertices);

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

	mesh.num_indices = SDL_arraysize(indices);
	mesh.indices = SDL_malloc(sizeof(mesh_index_t) * mesh.num_indices);
	SDL_memcpy(mesh.indices, indices, sizeof(mesh_index_t) * mesh.num_indices);

	return mesh;
}

void mesh_destroy(const mesh_info_t mesh)
{
	SDL_free(mesh.indices);
	SDL_free(mesh.vertices);
}
