#include "shapes.h"
#include "mesh.h"
#include "meshinfo.h"
#include "vector.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>

mesh_t *create_cube(SDL_GPUDevice *device, const vector3f_t size)
{
	const vector3f_t min = {
		.x = -(size.x / 2.F),
		.y = -(size.y / 2.F),
		.z = -(size.z / 2.F),
	};

	const vector3f_t max = {
		.x = (size.x / 2.F),
		.y = (size.y / 2.F),
		.z = (size.z / 2.F),
	};

	constexpr auto uv_tile_size = 10.F;
	const vector3f_t uv_max = {
		.x = size.x / uv_tile_size,
		.y = size.y / uv_tile_size,
		.z = size.z / uv_tile_size,
	};

	vertex_t vertices[] = {
		// Front (+z)
		(vertex_t){
			.position = (vector3f_t){min.x, min.y, max.z},
			.tex_coord = (vector2f_t){0.F, uv_max.y}
		},
		(vertex_t){
			.position = (vector3f_t){max.x, min.y, max.z},
			.tex_coord = (vector2f_t){uv_max.x, uv_max.y}
		},
		(vertex_t){
			.position = (vector3f_t){max.x, max.y, max.z},
			.tex_coord = (vector2f_t){uv_max.x, 0.F}
		},
		(vertex_t){
			.position = (vector3f_t){min.x, max.y, max.z},
			.tex_coord = (vector2f_t){0.F, 0.F}
		},
		// Back (-z)
		(vertex_t){
			.position = (vector3f_t){max.x, min.y, min.z},
			.tex_coord = (vector2f_t){0.F, uv_max.y}
		},
		(vertex_t){
			.position = (vector3f_t){min.x, min.y, min.z},
			.tex_coord = (vector2f_t){uv_max.x, uv_max.y}
		},
		(vertex_t){
			.position = (vector3f_t){min.x, max.y, min.z},
			.tex_coord = (vector2f_t){uv_max.x, 0.F}
		},
		(vertex_t){
			.position = (vector3f_t){max.x, max.y, min.z},
			.tex_coord = (vector2f_t){0.F, 0.F}
		},
		// Right (+x)
		(vertex_t){
			.position = (vector3f_t){max.x, min.y, max.z},
			.tex_coord = (vector2f_t){0.F, uv_max.y}
		},
		(vertex_t){
			.position = (vector3f_t){max.x, min.y, min.z},
			.tex_coord = (vector2f_t){uv_max.x, uv_max.y}
		},
		(vertex_t){
			.position = (vector3f_t){max.x, max.y, min.z},
			.tex_coord = (vector2f_t){uv_max.x, 0.F}
		},
		(vertex_t){
			.position = (vector3f_t){max.x, max.y, max.z},
			.tex_coord = (vector2f_t){0.F, 0.F}
		},
		// Left (-x)
		(vertex_t){
			.position = (vector3f_t){min.x, min.y, min.z},
			.tex_coord = (vector2f_t){0.F, uv_max.y}
		},
		(vertex_t){
			.position = (vector3f_t){min.x, min.y, max.z},
			.tex_coord = (vector2f_t){uv_max.x, uv_max.y}
		},
		(vertex_t){
			.position = (vector3f_t){min.x, max.y, max.z},
			.tex_coord = (vector2f_t){uv_max.x, 0.F}
		},
		(vertex_t){
			.position = (vector3f_t){min.x, max.y, min.z},
			.tex_coord = (vector2f_t){0.F, 0.F}
		},
		// Top (+y)
		(vertex_t){
			.position = (vector3f_t){min.x, max.y, max.z},
			.tex_coord = (vector2f_t){0.F, uv_max.z}
		},
		(vertex_t){
			.position = (vector3f_t){max.x, max.y, max.z},
			.tex_coord = (vector2f_t){uv_max.z, uv_max.z}
		},
		(vertex_t){
			.position = (vector3f_t){max.x, max.y, min.z},
			.tex_coord = (vector2f_t){uv_max.z, 0.F}
		},
		(vertex_t){
			.position = (vector3f_t){min.x, max.y, min.z},
			.tex_coord = (vector2f_t){0.F, 0.F}
		},
		// Bottom (-y)
		(vertex_t){
			.position = (vector3f_t){min.x, min.y, min.z},
			.tex_coord = (vector2f_t){0.F, uv_max.z}
		},
		(vertex_t){
			.position = (vector3f_t){max.x, min.y, min.z},
			.tex_coord = (vector2f_t){uv_max.z, uv_max.z}
		},
		(vertex_t){
			.position = (vector3f_t){max.x, min.y, max.z},
			.tex_coord = (vector2f_t){uv_max.z, 0.F}
		},
		(vertex_t){
			.position = (vector3f_t){min.x, min.y, max.z},
			.tex_coord = (vector2f_t){0.F, 0.F}
		},
	};

	const mesh_index_t indices[] = {
		0, 1, 2, 0, 2, 3,       // Front
		4, 5, 6, 4, 6, 7,       // Back
		8, 9, 10, 8, 10, 11,    // Right
		12, 13, 14, 12, 14, 15, // Left
		16, 17, 18, 16, 18, 19, // Top
		20, 21, 22, 20, 22, 23, // Bottom
	};

	const mesh_info_t mesh_info = {
		.num_vertices = SDL_arraysize(vertices),
		.vertices = vertices,
		.num_indices = SDL_arraysize(indices),
		.indices = indices,
	};

	return mesh_create(device, mesh_info);
}
