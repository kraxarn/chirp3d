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
	vector3f_t forward = camera_forward(camera);

	// We don't want to move up/down when looking up/down
	forward.y = 0;
	forward = vector3f_normalize(forward);

	const vector3f_t scaled = vector3f_scale(forward, movement);

	camera->position = vector3f_add(camera->position, scaled);
	camera->target = vector3f_add(camera->target, scaled);
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

void camera_rotate_y(camera_t *camera, const float angle) // pitch
{
	const vector3f_t up = camera_up(camera);
	vector3f_t target = vector3f_sub(camera->target, camera->position);

	// To avoid numerical errors
	constexpr auto offset = 0.001F;

	const float max_up = vector3f_angle(up, target) - offset;
	const float max_down = -vector3f_angle(vector3f_invert(up), target) + offset;
	const float clamped_angle = SDL_min(SDL_max(angle, max_down), max_up);

	const vector3f_t right = camera_right(camera);
	target = vector3f_rotate(target, right, clamped_angle);

	camera->target = vector3f_add(camera->position, target);
}
