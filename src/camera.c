#include "camera.h"

camera_t camera_create_default()
{
	return (camera_t){
		.position = (vector3f_t){.x = 0.F, .y = 10.F, .z = -30.F},
		.target = (vector3f_t){.x = 0.F, .y = 10.F, .z = 0.F},
		.up = (vector3f_t){.x = 0.F, .y = 1.F, .z = 0.F},
		.fov_y = 70.F,
		.near_plane = 0.05F,
		.far_plane = 4'000.F,
	};
}

void camera_move_forward(camera_t *camera, const float movement)
{
	const vector3f_t forward = vector3f_normalize(vector3f_sub(camera->target, camera->position));
	const vector3f_t move = vector3f_scale(forward, movement);

	camera->position = vector3f_add(camera->position, move);
	camera->target = vector3f_add(camera->target, move);
}
