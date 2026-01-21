#include "systeminfo.h"

#include "cpuinfo.h"

#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_version.h>
#include <SDL3/SDL_platform.h>
#include <SDL3/SDL_cpuinfo.h>

static constexpr size_t name_length = 64;

const char *system_info_platform()
{
	return SDL_GetPlatform();
}

const char *system_info_cpu_name()
{
	static char cpu_name[name_length];
	if (cpu_name[0] != '\0')
	{
		return cpu_name;
	}

	if (!cpuinfo_initialize())
	{
		return nullptr;
	}

	SDL_strlcpy(cpu_name, cpuinfo_get_package(0)->name, name_length);

	cpuinfo_deinitialize();
	return cpu_name;
}

bool system_info_cpu_supported()
{
#if CPUINFO_ARCH_X86_64
	if (!SDL_HasAVX2() || !SDL_HasSSE42())
	{
		return SDL_SetError("CPU doesn't support required AVX2 and SSE4.2 features");
	}
#elif CPUINFO_ARCH_ARM64
	if (!SDL_HasNEON())
	{
		return SDL_SetError("CPU doesn't support required NEON features");
	}
#else
#error Unknown CPU architecture
#endif

	return true;
}

const char *system_info_gpu_name(SDL_GPUDevice *device)
{
	static char gpu_name[name_length];
	if (gpu_name[0] != '\0')
	{
		return gpu_name;
	}

#if SDL_VERSION_ATLEAST(3, 4, 0)
	const SDL_PropertiesID props = SDL_GetGPUDeviceProperties(device);

	const char *name = SDL_GetStringProperty(props,
		SDL_PROP_GPU_DEVICE_NAME_STRING, nullptr);

	if (name == nullptr)
	{
		return nullptr;
	}

	SDL_strlcpy(gpu_name, name, name_length);
	return gpu_name;
#else
	return nullptr;
#endif
}

const char *system_info_gpu_driver(SDL_GPUDevice *device)
{
	static char gpu_driver_name[name_length];
	if (gpu_driver_name[0] != '\0')
	{
		return gpu_driver_name;
	}

#if SDL_VERSION_ATLEAST(3, 4, 0)
	const SDL_PropertiesID props = SDL_GetGPUDeviceProperties(device);

	const char *name = SDL_GetStringProperty(props,
		SDL_PROP_GPU_DEVICE_DRIVER_NAME_STRING, nullptr);

	if (name == nullptr)
	{
		return nullptr;
	}

	SDL_strlcpy(gpu_driver_name, name, name_length);

	const char *version = SDL_GetStringProperty(props,
		SDL_PROP_GPU_DEVICE_DRIVER_INFO_STRING, SDL_GetStringProperty(props,
			SDL_PROP_GPU_DEVICE_DRIVER_VERSION_STRING, nullptr));

	if (version == nullptr)
	{
		return nullptr;
	}

	SDL_strlcat(gpu_driver_name, " ", name_length);
	SDL_strlcat(gpu_driver_name, version, name_length);
	return gpu_driver_name;

#else
	return nullptr;
#endif
}
