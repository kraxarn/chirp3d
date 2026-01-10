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

vector3f_t camera_to_z(const camera_t *camera, const float movement)
{
	vector3f_t forward = camera_forward(camera);

	// We don't want to move up/down when looking up/down
	forward.y = 0;
	forward = vector3f_normalize(forward);

	return vector3f_scale(forward, movement);
}

vector3f_t camera_to_x(const camera_t *camera, const float movement)
{
	const vector3f_t right = camera_right(camera);
	return vector3f_scale(right, movement);
}

vector3f_t camera_to_y(const camera_t *camera, const float movement)
{
	const vector3f_t up = camera_up(camera);
	return vector3f_scale(up, movement);
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
