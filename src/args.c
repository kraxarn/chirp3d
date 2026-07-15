#include "args.h"
#include "logcategory.h"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

args_t args_parse(const int argc, char **argv)
{
	args_t args = {
		.prefer_low_power = OPT_NOT_SET,
		.gpu_debug_mode = OPT_NOT_SET,
	};

	for (int i = 0; i < argc; i++)
	{
		const char *arg = argv[i];
		if (SDL_strcmp(arg, "--prefer-low-power") == 0)
		{
			args.prefer_low_power = OPT_ENABLE;
		}
		else if (SDL_strcmp(arg, "--no-prefer-low-power") == 0)
		{
			args.prefer_low_power = OPT_DISABLE;
		}
		else if (SDL_strcmp(arg, "--gpu-debug-mode") == 0)
		{
			args.gpu_debug_mode = OPT_ENABLE;
		}
		else if (SDL_strcmp(arg, "--no-gpu-debug-mode") == 0)
		{
			args.gpu_debug_mode = OPT_DISABLE;
		}
		else
		{
			SDL_LogWarn(LOG_CATEGORY_CORE,
				"Unknown arg: '%s'", arg);
		}
	}

	return args;
}
