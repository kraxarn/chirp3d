#include "mesh.h"

size_t mesh_upload_size(const mesh_t mesh)
{
	return mesh_vertex_size(mesh) + mesh_index_size(mesh);
}

size_t mesh_vertex_size(const mesh_t mesh)
{
	return sizeof(vertex_t) * mesh.num_vertices;
}

size_t mesh_index_size(const mesh_t mesh)
{
	return sizeof(mesh_index_t) * mesh.num_indices;
}
