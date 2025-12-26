#pragma once

#include "vector.h"

static constexpr int matrix4x4_size = 4 * 4;

typedef struct matrix4x4_t
{
	float m[matrix4x4_size];
} matrix4x4_t;

[[nodiscard]]
matrix4x4_t matrix4x4_multiply(matrix4x4_t mat1, matrix4x4_t mat2);

[[nodiscard]]
matrix4x4_t matrix4x4_create_rotation_x(float radians);

[[nodiscard]]
matrix4x4_t matrix4x4_create_rotation_y(float radians);

[[nodiscard]]
matrix4x4_t matrix4x4_create_rotation_z(float radians);

[[nodiscard]]
matrix4x4_t matrix4x4_create_translation(vector3f_t vec);

[[nodiscard]]
matrix4x4_t matrix4x4_create_perspective(float fov, float aspect, float near, float far);

[[nodiscard]]
matrix4x4_t matrix4x4_create_look_at(vector3f_t camera_position, vector3f_t camera_target, vector3f_t camera_up);
