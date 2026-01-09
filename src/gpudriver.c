#include "gpudriver.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>

char *gpu_driver_names()
{
	// "vulkan, metal, direct3d12"
	constexpr size_t gpu_drivers_len = 25;

	char *gpu_drivers = SDL_malloc(sizeof(char) * (gpu_drivers_len + 1));
	if (gpu_drivers == nullptr)
	{
		return nullptr;
	}
	gpu_drivers[0] = '\0';

	for (auto i = 0; i < SDL_GetNumGPUDrivers(); i++)
	{
		SDL_strlcat(gpu_drivers, SDL_GetGPUDriver(i), gpu_drivers_len);
		if (i < SDL_GetNumGPUDrivers() - 1)
		{
			SDL_strlcat(gpu_drivers, ", ", gpu_drivers_len);
		}
	}

	gpu_drivers[SDL_strlen(gpu_drivers)] = '\0';
	return gpu_drivers;
}
