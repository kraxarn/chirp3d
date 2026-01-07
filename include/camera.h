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

/**
 * Move camera in the Z-axis (forwards/backwards)
 * @param camera Camera to modify
 * @param movement Forward movement
 */
void camera_move_z(camera_t *camera, float movement);

/**
 * Move camera in the X-axis (left/right)
 * @param camera Camera to modify
 * @param movement Left movement
 */
void camera_move_x(camera_t *camera, float movement);
