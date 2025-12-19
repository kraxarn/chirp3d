#include "math.h"

#include <SDL3/SDL_stdinc.h>

float clamp(const float value, const float min, const float max)
{
	return SDL_min(max, SDL_max(min, value));
}

float lerp(const float start, const float end, const float amount)
{
	return start + (amount * (end - start));
}

float normalize(const float value, const float start, const float end)
{
	return (value - start) / (end - start);
}

float remap(const float value, const float in_start, const float in_end, const float out_start, const float out_end)
{
	return ((value - in_start) / (in_end - in_start) * (out_end - out_start)) + out_start;
}

float wrap(const float value, const float min, const float max)
{
	return value - ((max - min) * SDL_floorf(((value - min) / max) - min));
}

bool eqf(const float value1, const float value2)
{
	return SDL_fabsf(value1 - value2) <= SDL_FLT_EPSILON * SDL_max(1.F, SDL_max(SDL_fabsf(value1), SDL_fabsf(value2)));
}
