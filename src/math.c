#include "math.h"

#include <SDL3/SDL_stdinc.h>

float deg2rad(const float degrees)
{
	return degrees * SDL_PI_F / 180.F;
}
