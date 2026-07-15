#include "args.h"
#include "logcategory.h"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

[[nodiscard]]
static SDL_LogPriority parse_log_priority(const char *name)
{
	// There are technically more categories,
	// but these are the "main ones" i feel

	if (SDL_strcmp(name, "debug") == 0)
	{
		return SDL_LOG_PRIORITY_DEBUG;
	}

	if (SDL_strcmp(name, "info") == 0)
	{
		return SDL_LOG_PRIORITY_INFO;
	}

	if (SDL_strcmp(name, "warn") == 0)
	{
		return SDL_LOG_PRIORITY_WARN;
	}

	if (SDL_strcmp(name, "error") == 0)
	{
		return SDL_LOG_PRIORITY_ERROR;
	}

	return SDL_LOG_PRIORITY_INVALID;
}

args_t args_parse(const int argc, char **argv)
{
	args_t args = {
		.prefer_low_power = OPT_NOT_SET,
		.gpu_debug_mode = OPT_NOT_SET,
		.log_priority = SDL_LOG_PRIORITY_INVALID,
		.video_driver = nullptr,
	};

	for (int i = 1; i < argc; i++)
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

		else if (SDL_strcmp(arg, "--log-priority") == 0 && i + 1 < argc)
		{
			args.log_priority = parse_log_priority(argv[++i]);
			if (args.log_priority == SDL_LOG_PRIORITY_INVALID)
			{
				SDL_LogError(LOG_CATEGORY_CORE, "Unknown priority: '%s'", argv[i]);
			}
		}

		else if (SDL_strcmp(arg, "--video-driver") == 0 && i + 1 < argc)
		{
			args.video_driver = argv[++i];
		}

		else
		{
			SDL_LogError(LOG_CATEGORY_CORE, "Unknown arg: '%s'", arg);
		}
	}

	return args;
}
