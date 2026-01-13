#include "cpuinfo.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_platform_defines.h>

#ifdef SDL_PLATFORM_APPLE
#include <sys/errno.h>
#include <sys/sysctl.h>
#endif

#ifdef SDL_PLATFORM_UNIX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#ifdef SDL_PLATFORM_UNIX
	// TODO: This only works on x86
	FILE *file = fopen("/proc/cpuinfo", "r");
	if (file == nullptr)
	{
		SDL_SetError("File not found");
		return nullptr;
	}
	const char *key = "model name";
	char *line = nullptr;
	size_t len = 0;
	while (getline(&line, &len, file) >= 0)
	{
		if (strncmp(line, key, strlen(key)) != 0)
		{
			continue;
		}
		const char *sep = strchr(line, ':');
		if (sep == nullptr)
		{
			continue;
		}
		strncpy(cpu_name, sep + 2, cpu_name_len);
		free(line);
		fclose(file);
		return cpu_name;
	}
	free(line);
	fclose(file);
	return nullptr;
#endif

	return "Unknown";
}
