#pragma once

#include <SDL3/SDL_stdinc.h>

constexpr auto std140 = 16;

typedef struct vector2f_t
{
	float x;
	float y;
} vector2f_t;

typedef struct vector2f_aligned_t
{
	alignas(std140)
	float x;
	float y;
} vector2f_aligned_t;

typedef struct vector2i_t
{
	int x;
	int y;
} vector2i_t;

typedef struct vector3f_t
{
	float x;
	float y;
	float z;
} vector3f_t;

typedef struct vector4f_t
{
	float x;
	float y;
	float z;
	float w;
} vector4f_t;

[[nodiscard]]
vector3f_t vector3f_zero();

[[nodiscard]]
vector3f_t vector3f_add(vector3f_t vec1, vector3f_t vec2);

[[nodiscard]]
vector3f_t vector3f_sub(vector3f_t vec1, vector3f_t vec2);

[[nodiscard]]
vector3f_t vector3f_scale(vector3f_t vec, float scalar);

[[nodiscard]]
vector3f_t vector3f_normalize(vector3f_t vec);

[[nodiscard]]
float vector3f_dot(vector3f_t vec1, vector3f_t vec2);

[[nodiscard]]
vector3f_t vector3f_cross(vector3f_t vec1, vector3f_t vec2);

[[nodiscard]]
vector3f_t vector3f_rotate(vector3f_t vec, vector3f_t axis, float angle);

char *vector3f_str(vector3f_t vec, char *str, size_t max_len);
