#include "cpuinfo.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_platform_defines.h>

#ifdef SDL_PLATFORM_APPLE
#include <sys/errno.h>
#include <sys/sysctl.h>
#endif

const char *cpu_name()
{
	static constexpr size_t cpu_name_len = 64;
	static char cpu_name[cpu_name_len];

#ifdef SDL_PLATFORM_APPLE
	size_t data_len = sizeof(cpu_name);
	if (sysctlbyname("machdep.cpu.brand_string", cpu_name, &data_len, nullptr, 0) == 0)
	{
		return cpu_name;
	}
	SDL_SetError("Error %d", errno);
	return nullptr;
#endif

	return "Unknown";
}
