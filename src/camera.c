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

static vector3f_t camera_forward(const camera_t *camera)
{
	return vector3f_normalize(vector3f_sub(camera->target, camera->position));
}

static vector3f_t camera_up(const camera_t *camera)
{
	return vector3f_normalize(camera->up);
}

static vector3f_t camera_left(const camera_t *camera)
{
	return vector3f_normalize(vector3f_cross(camera_forward(camera), camera_up(camera)));
}

void camera_move_z(camera_t *camera, const float movement)
{
	const vector3f_t forward = vector3f_scale(camera_forward(camera), movement);

	camera->position = vector3f_add(camera->position, forward);
	camera->target = vector3f_add(camera->target, forward);
}

void camera_move_x(camera_t *camera, const float movement)
{
	const vector3f_t left = vector3f_scale(camera_left(camera), movement);

	camera->position = vector3f_add(camera->position, left);
	camera->target = vector3f_add(camera->target, left);
}

void camera_move_y(camera_t *camera, const float movement)
{
	const vector3f_t up = vector3f_scale(camera_up(camera), movement);

	camera->position = vector3f_add(camera->position, up);
	camera->target = vector3f_add(camera->target, up);
}
