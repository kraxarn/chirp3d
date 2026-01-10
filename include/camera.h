#pragma once

#include "vector.h"

typedef struct camera_t
{
	vector3f_t position;
	vector3f_t target;
	vector3f_t up;
	float fov_y;
	float near_plane;
	float far_plane;
} camera_t;

[[nodiscard]]
camera_t camera_create_default();

[[nodiscard]]
vector3f_t camera_to_z(const camera_t *camera, float movement);

[[nodiscard]]
vector3f_t camera_to_x(const camera_t *camera, float movement);

[[nodiscard]]
vector3f_t camera_to_y(const camera_t *camera, float movement);

/**
 * Camera jaw
 */
void camera_rotate_x(camera_t *camera, float angle);

/**
 * Camera pitch
 */
void camera_rotate_y(camera_t *camera, float angle);
