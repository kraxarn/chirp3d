#include "model.h"
#include "mesh.h"
#include "meshinfo.h"
#include "vector.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>

#define PAR_MALLOC(T, N) ((T*) SDL_malloc(N * sizeof(T)))
#define PAR_CALLOC(T, N) ((T*) SDL_calloc(N * sizeof(T), 1))
#define PAR_REALLOC(T, BUF, N) ((T*) SDL_realloc(BUF, sizeof(T) * (N)))
#define PAR_FREE(BUF) SDL_free(BUF)

#define sqrt(x)  SDL_sqrt(x)
#define cosf(x)  SDL_cosf(x)
#define sinf(x)  SDL_sinf(x)
#define cos(x)   SDL_cos(x)
#define sin(x)   SDL_sin(x)
#define acos(x)  SDL_acos(x)
#define pow(x,y) SDL_pow(x,y)

#define PAR_SHAPES_IMPLEMENTATION
#include "par_shapes.h"

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

mesh_t *create_sphere(SDL_GPUDevice *device, const float radius)
{
	par_shapes_set_epsilon_degenerate_sphere(0.F);

	// TODO: Maybe allow setting these manually
	constexpr auto slices = 32;
	constexpr auto stacks = 32;

	par_shapes_mesh *sphere = par_shapes_create_parametric_sphere(slices, stacks);
	par_shapes_scale(sphere, radius, radius, radius);

	const auto num_vertices = (size_t) sphere->npoints;
	const size_t num_indices = (size_t) sphere->ntriangles * 3;

	const auto points = (vector3f_t *) sphere->points;
	const auto tex_coords = (vector2f_t *) sphere->tcoords;
	const auto indices = (vector3u16_t *) sphere->triangles;

	vertex_t *vertices = SDL_malloc(num_vertices * sizeof(vertex_t));

	for (size_t i = 0; i < num_vertices; i++)
	{
		SDL_assert(points[i].x == sphere->points[(i * 3) + 0]);
		SDL_assert(points[i].y == sphere->points[(i * 3) + 1]);
		SDL_assert(points[i].z == sphere->points[(i * 3) + 2]);

		SDL_assert(tex_coords[i].x == sphere->tcoords[(i * 2) + 0]);
		SDL_assert(tex_coords[i].y == sphere->tcoords[(i * 2) + 1]);

		vertices[i].position = points[i];
		vertices[i].tex_coord = tex_coords[i];
	}

	for (size_t i = 0; i < num_indices / 3; i++)
	{
		SDL_assert(indices[i].x == sphere->triangles[(i * 3) + 0]);
		SDL_assert(indices[i].y == sphere->triangles[(i * 3) + 1]);
		SDL_assert(indices[i].z == sphere->triangles[(i * 3) + 2]);
	}

	const mesh_info_t mesh_info = {
		.num_vertices = num_vertices,
		.vertices = vertices,
		.num_indices = sphere->ntriangles,
		.indices = sphere->triangles,
	};

	mesh_t *mesh = mesh_create(device, mesh_info);

	SDL_free(vertices);
	par_shapes_free_mesh(sphere);

	return mesh;
}
