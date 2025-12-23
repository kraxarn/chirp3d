#pragma once

typedef struct vector3f_t
{
	float x;
	float y;
	float z;
} vector3f_t;

[[nodiscard]]
vector3f_t vector3f_zero();
