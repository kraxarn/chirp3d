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

void camera_move_forward(camera_t *camera, float movement);
