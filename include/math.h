#pragma once

typedef struct vector2f_t
{
	float x;
	float y;
} vector2f_t;

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

typedef struct matrix4x4_t
{
	float m[16];
} matrix4x4_t;

[[nodiscard]]
float clamp(float value, float min, float max);

[[nodiscard]]
float lerp(float start, float end, float amount);

[[nodiscard]]
float normalize(float value, float start, float end);

[[nodiscard]]
float remap(float value, float in_start, float in_end, float out_start, float out_end);

[[nodiscard]]
float wrap(float value, float min, float max);

[[nodiscard]]
bool eqf(float value1, float value2);
