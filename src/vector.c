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

vector3f_t vector3f_rotate(const vector3f_t vec, vector3f_t axis, float angle)
{
	vector3f_t result = vec;

	// normalize(axis)
	float length = SDL_sqrtf((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z));
	if (length == 0.F)
	{
		length = 1.F;
	}
	const float i_length = 1.F / length;
	axis.x *= i_length;
	axis.y *= i_length;
	axis.z *= i_length;

	angle /= 2.F;
	float a = SDL_sinf(angle);
	const float b = axis.x * a;
	const float c = axis.y * a;
	const float d = axis.z * a;
	a = SDL_cosf(angle);
	vector3f_t w = {.x = b, .y = c, .z = d};

	// cross(w, v)
	vector3f_t wv = {
		.x = (w.y * vec.z) - (w.z * vec.y),
		.y = (w.z * vec.x) - (w.x * vec.z),
		.z = (w.x * vec.y) - (w.y * vec.x),
	};

	// cross(w, wv)
	vector3f_t wwv = {
		.x = (w.y * wv.z) - (w.z * wv.y),
		.y = (w.z * wv.x) - (w.x * wv.z),
		.z = (w.x * wv.y) - (w.y * wv.x),
	};

	// scale(wv, 2 * a)
	a *= 2;
	wv.x *= a;
	wv.y *= a;
	wv.z *= a;

	// scale(wwv, 2)
	wwv.x *= 2;
	wwv.y *= 2;
	wwv.z *= 2;

	result.x += wv.x;
	result.y += wv.y;
	result.z += wv.z;

	result.x += wwv.x;
	result.y += wwv.y;
	result.z += wwv.z;

	return result;
}

char *vector3f_str(const vector3f_t vec, char *str, const size_t max_len)
{
	SDL_snprintf(str, max_len, "%6.2f %6.2f %6.2f", vec.x, vec.y, vec.z);
	return str;
}
