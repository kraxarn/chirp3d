#pragma once

#include "vector.h"

#include <SDL3/SDL_stdinc.h>

typedef Uint16 mesh_index_t;

typedef struct vertex_t
{
	vector3f_t position;
	vector2f_t tex_coord;
} vertex_t;

typedef struct mesh_info_t
{
	const vertex_t *vertices;
	const size_t num_vertices;

	const mesh_index_t *indices;
	const size_t num_indices;
} mesh_info_t;
