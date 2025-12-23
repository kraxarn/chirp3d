#include "matrix.h"

#include <SDL3/SDL_stdinc.h>

matrix4x4_t matrix4x4_multiply(const matrix4x4_t mat1, const matrix4x4_t mat2)
{
	matrix4x4_t result;

	for (auto i = 0; i < 4; i++)
	{
		for (auto j = 0; j < 4; j++)
		{
			result.m[(i * 4) + j] = 0.F;

			for (auto k = 0; k < 4; k++)
			{
				result.m[(i * 4) + j] += mat1.m[(i * 4) + k] * mat2.m[(k * 4) + j];
			}
		}
	}

	return result;
}

matrix4x4_t matrix4x4_create_rotation_z(const float radians)
{
	return (matrix4x4_t){
		SDL_cosf(radians), SDL_sinf(radians), 0, 0,
		-SDL_sinf(radians), SDL_cosf(radians), 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};
}

matrix4x4_t matrix4x4_create_translation(const vector3f_t vec)
{
	return (matrix4x4_t){
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		vec.x, vec.y, vec.z, 1,
	};
}

matrix4x4_t matrix4x4_create_perspective(const float fov, const float aspect, const float near, const float far)
{
	const float num = 1.F / SDL_tanf(fov * 0.5F);

	return (matrix4x4_t){
		num / aspect, 0, 0, 0,
		0, num, 0, 0,
		0, 0, far / (near - far), -1,
		0, 0, (near * far) / (near - far), 0
	};
}

matrix4x4_t matrix4x4_create_look_at(const vector3f_t camera_position,
	const vector3f_t camera_target, const vector3f_t camera_up)
{
	const vector3f_t target = {
		.x = camera_position.x - camera_target.x,
		.y = camera_position.y - camera_target.y,
		.z = camera_position.z - camera_target.z
	};
	const vector3f_t vec1 = vector3f_normalize(target);
	const vector3f_t vec2 = vector3f_normalize(vector3f_cross(camera_up, vec1));
	const vector3f_t vec3 = vector3f_cross(vec1, vec2);

	return (matrix4x4_t){
		vec2.x, vec3.x, vec1.x, 0,
		vec2.y, vec3.y, vec1.y, 0,
		vec2.z, vec3.z, vec1.z, 0,
		-vector3f_dot(vec2, camera_position), -vector3f_dot(vec3, camera_position),
		-vector3f_dot(vec1, camera_position), 1
	};
}
