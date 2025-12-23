#include "model.h"
#include "gpumesh.h"
#include "mesh.h"
#include "vector.h"

#include <SDL3/SDL_gpu.h>

mesh_t *create_cube(SDL_GPUDevice *device, [[maybe_unused]] const vector3f_t position,
	[[maybe_unused]] const vector3f_t size)
{
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

	return gpu_mesh_create(device, mesh_info);
}
