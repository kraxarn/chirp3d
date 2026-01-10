#include "vector.h"

#include <SDL3/SDL_stdinc.h>

vector3f_t vector3f_zero()
{
	return (vector3f_t){
		.x = 0.F,
		.y = 0.F,
		.z = 0.F,
	};
}

vector3f_t vector3f_add(const vector3f_t vec1, const vector3f_t vec2)
{
	return (vector3f_t){
		.x = vec1.x + vec2.x,
		.y = vec1.y + vec2.y,
		.z = vec1.z + vec2.z,
	};
}

vector3f_t vector3f_sub(const vector3f_t vec1, const vector3f_t vec2)
{
	return (vector3f_t){
		.x = vec1.x - vec2.x,
		.y = vec1.y - vec2.y,
		.z = vec1.z - vec2.z,
	};
}

vector3f_t vector3f_scale(const vector3f_t vec, const float scalar)
{
	return (vector3f_t){
		.x = vec.x * scalar,
		.y = vec.y * scalar,
		.z = vec.z * scalar,
	};
}

vector3f_t vector3f_normalize(const vector3f_t vec)
{
	const float magnitude = SDL_sqrtf((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z));
	return (vector3f_t){
		.x = vec.x / magnitude,
		.y = vec.y / magnitude,
		.z = vec.z / magnitude,
	};
}

float vector3f_dot(const vector3f_t vec1, const vector3f_t vec2)
{
	return (vec1.x * vec2.x) + (vec1.y * vec2.y) + (vec1.z * vec2.z);
}

vector3f_t vector3f_cross(const vector3f_t vec1, const vector3f_t vec2)
{
	return (vector3f_t){
		.x = (vec1.y * vec2.z) - (vec2.y * vec1.z),
		.y = -((vec1.x * vec2.z) - (vec2.x * vec1.z)),
		.z = (vec1.x * vec2.y) - (vec2.x * vec1.y),
	};
}

vector3f_t vector3f_rotate(const vector3f_t vec, const vector3f_t axis, const float angle)
{
	const vector3f_t axis_norm = vector3f_normalize(axis);
	const vector3f_t axis_scaled = vector3f_scale(axis_norm, SDL_sinf(angle));
	const vector3f_t cross1 = vector3f_scale(vector3f_cross(axis_scaled, vec), SDL_cosf(angle) * 2);
	const vector3f_t cross2 = vector3f_scale(vector3f_cross(axis_scaled, cross1), 2);

	return vector3f_add(vec, vector3f_add(cross1, cross2));
}

float vector3f_angle(const vector3f_t vec1, const vector3f_t vec2)
{
	// TODO: Clean this up
	const vector3f_t cross = {
		.x = (vec1.y * vec2.z) - (vec1.z * vec2.y),
		.y = (vec1.z * vec2.x) - (vec1.x * vec2.z),
		.z = (vec1.x * vec2.y) - (vec1.y * vec2.x),
	};
	const float len = SDL_sqrtf((cross.x * cross.x) + (cross.y * cross.y) + (cross.z * cross.z));
	const float dot = (vec1.x * vec2.x) + (vec1.y * vec2.y) + (vec1.z * vec2.z);

	return SDL_atan2f(len, dot);
}

vector3f_t vector3f_invert(const vector3f_t vec)
{
	return (vector3f_t){
		.x = -vec.x,
		.y = -vec.y,
		.z = -vec.z,
	};
}
