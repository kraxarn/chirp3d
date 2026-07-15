#include "args.h"
#include "logcategory.h"

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>

typedef struct
{
	const char *command;
	const char *description;
} arg_command_t;

static void print_help()
{
	// To avoid custom log formatting
	// (no need to clean-up since we should exit immediately afterwards)
	SDL_SetLogPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);
	SDL_SetLogPriorityPrefix(SDL_LOG_PRIORITY_INFO, "");
	const SDL_LogOutputFunction log_func = SDL_GetDefaultLogOutputFunction();

	constexpr size_t video_drivers_len = 256;
	char video_drivers[video_drivers_len];
	video_drivers[0] = '\0';
	for (int i = 0; i < SDL_GetNumVideoDrivers(); i++)
	{
		SDL_strlcat(video_drivers, i == 0 ? "--video-driver [" : "/", video_drivers_len);
		SDL_strlcat(video_drivers, SDL_GetVideoDriver(i), video_drivers_len);
	}
	SDL_strlcat(video_drivers, "]", video_drivers_len);

	const arg_command_t commands[] = {
		(arg_command_t){
			.command = "--help",
			.description = "Show this"
		},
		(arg_command_t){
			.command = "--(no-)prefer-low-power",
			.description = "Prefer integrated GPU over dedicated GPU",
		},
		(arg_command_t){
			.command = "--(no-)gpu-debug-mode",
			.description = "Enable GPU validations and more verbose GPU logging",
		},
		(arg_command_t){
			.command = "--log-priority [debug/info/warn/error]",
			.description = "Set global logging verbosity",
		},
		(arg_command_t){
			.command = video_drivers,
			.description = "Force specific video driver",
		},
		(arg_command_t){
			.command = "--(no-)allow-screensaver",
			.description = "Enable screensaver while running",
		},
		(arg_command_t){
			.command = "--(no-)fatal-error-message-box",
			.description = "Show a message box on fatal error",
		},
	};

	log_func(nullptr, SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO,
		"Usage: " ENGINE_NAME " [options]");

	int command_length = 0;
	for (size_t i = 0; i < SDL_arraysize(commands); i++)
	{
		command_length = SDL_max(command_length, SDL_strlen(commands[i].command));
	}

	for (size_t i = 0; i < SDL_arraysize(commands); i++)
	{
		const arg_command_t cmd = commands[i];
		char *temp = nullptr;
		SDL_asprintf(&temp, "  %-*s %s", command_length, cmd.command, cmd.description);
		log_func(nullptr, SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, temp);
		SDL_free(temp);
	}
}

[[nodiscard]]
static SDL_LogPriority parse_log_priority(const char *name)
{
	// There are technically more categories,
	// but these are the "main ones" I feel

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

bool args_parse(const int argc, char **argv, args_t *args)
{
	*args = (args_t){
		.prefer_low_power = OPT_NOT_SET,
		.gpu_debug_mode = OPT_NOT_SET,
		.log_priority = SDL_LOG_PRIORITY_INVALID,
		.video_driver = nullptr,
	};

	for (int i = 1; i < argc; i++)
	{
		const char *arg = argv[i];

		if (SDL_strcmp(arg, "--help") == 0)
		{
			print_help();
			return false;
		}

		if (SDL_strcmp(arg, "--prefer-low-power") == 0)
		{
			args->prefer_low_power = OPT_ENABLE;
		}
		else if (SDL_strcmp(arg, "--no-prefer-low-power") == 0)
		{
			args->prefer_low_power = OPT_DISABLE;
		}

		else if (SDL_strcmp(arg, "--gpu-debug-mode") == 0)
		{
			args->gpu_debug_mode = OPT_ENABLE;
		}
		else if (SDL_strcmp(arg, "--no-gpu-debug-mode") == 0)
		{
			args->gpu_debug_mode = OPT_DISABLE;
		}

		else if (SDL_strcmp(arg, "--log-priority") == 0 && i + 1 < argc)
		{
			args->log_priority = parse_log_priority(argv[++i]);
			if (args->log_priority == SDL_LOG_PRIORITY_INVALID)
			{
				SDL_LogError(LOG_CATEGORY_CORE, "Unknown priority: '%s'", argv[i]);
			}
		}

		else if (SDL_strcmp(arg, "--video-driver") == 0 && i + 1 < argc)
		{
			args->video_driver = argv[++i];
		}

		else if (SDL_strcmp(arg, "--allow-screensaver") == 0)
		{
			args->allow_screensaver = OPT_ENABLE;
		}
		else if (SDL_strcmp(arg, "--no-allow-screensaver") == 0)
		{
			args->allow_screensaver = OPT_DISABLE;
		}

		else if (SDL_strcmp(arg, "--fatal-error-message-box") == 0)
		{
			args->fatal_error_message_box = OPT_ENABLE;
		}
		else if (SDL_strcmp(arg, "--no-fatal-error-message-box") == 0)
		{
			args->fatal_error_message_box = OPT_DISABLE;
		}

		else
		{
			SDL_LogError(LOG_CATEGORY_CORE, "Unknown arg: '%s'", arg);
		}
	}

	return true;
}

const char *arg_option_str(const arg_option_t option)
{
	switch (option)
	{
		case OPT_NOT_SET:
			return "-1";

		case OPT_DISABLE:
			return "0";

		case OPT_ENABLE:
			return "1";

		default:
			return "";
	}
}
