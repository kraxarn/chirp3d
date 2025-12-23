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

vector3f_t vector3f_sub(const vector3f_t vec1, const vector3f_t vec2)
{
	return (vector3f_t){
		.x = vec1.x - vec2.x,
		.y = vec1.y - vec2.y,
		.z = vec1.z - vec2.z,
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
