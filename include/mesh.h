#pragma once

#include "vector.h"

#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_stdinc.h>

typedef Uint16 mesh_index_t;

typedef struct vertex_t
{
	vector3f_t position;
	SDL_FColor color;
} vertex_t;

typedef struct mesh_info_t
{
	vertex_t *vertices;
	size_t num_vertices;

	mesh_index_t *indices;
	size_t num_indices;
} mesh_info_t;

[[nodiscard]]
size_t mesh_upload_size(mesh_info_t mesh);

[[nodiscard]]
size_t mesh_vertex_size(mesh_info_t mesh);

[[nodiscard]]
size_t mesh_index_size(mesh_info_t mesh);
