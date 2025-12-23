#pragma once

#include "mesh.h"
#include "vector.h"

[[nodiscard]]
mesh_info_t create_cube(vector3f_t position, vector3f_t size);

void mesh_destroy(mesh_info_t mesh);
