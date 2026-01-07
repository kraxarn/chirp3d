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

static vector3f_t camera_right(const camera_t *camera)
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
	const vector3f_t right = vector3f_scale(camera_right(camera), movement);

	camera->position = vector3f_add(camera->position, right);
	camera->target = vector3f_add(camera->target, right);
}

void camera_move_y(camera_t *camera, const float movement)
{
	const vector3f_t up = vector3f_scale(camera_up(camera), movement);

	camera->position = vector3f_add(camera->position, up);
	camera->target = vector3f_add(camera->target, up);
}

void camera_rotate_x(camera_t *camera, const float angle) // jaw
{
	const vector3f_t up = camera_up(camera);
	const vector3f_t target = vector3f_sub(camera->target, camera->position);
	const vector3f_t rotated = vector3f_rotate(target, up, angle);

	camera->target = vector3f_add(camera->position, rotated);
}

void camera_rotate_y(camera_t *camera, float angle) // patch
{
	const vector3f_t up = camera_up(camera);
	vector3f_t target = vector3f_sub(camera->target, camera->position);

	// TODO: Use proper clamping and clean this up

	float max_up = vector3f_angle(up, target);
	max_up -= 0.001F;
	if (angle > max_up)
	{
		angle = max_up;
	}

	float max_down = vector3f_angle(vector3f_invert(up), target);
	max_down *= -1.F;
	max_down += 0.001F;
	if (angle < max_down)
	{
		angle = max_down;
	}

	vector3f_t right = camera_right(camera);
	target = vector3f_rotate(target, right, angle);

	camera->target = vector3f_add(camera->position, target);
}
