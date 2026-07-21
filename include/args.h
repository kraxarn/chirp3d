#pragma once

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

typedef enum : Uint8
{
	OPT_NOT_SET, // Not explicitly set
	OPT_DISABLE, // Explicitly disabled
	OPT_ENABLE,  // Explicitly enabled
} arg_option_t;

typedef struct
{
	/**
	 * --prefer-low-power / --no-prefer-low-power
	 *
	 * Create the GPU device with the "prefer low power" hint
	 */
	arg_option_t prefer_low_power;

	/**
	 * --gpu-debug-mode / --no-gpu-debug-mode
	 *
	 * Create the GPU device with the "debug mode" and "verbose" hints
	 */
	arg_option_t gpu_debug_mode;

	/**
	 * --log-priority debug/info/warn/error
	 *
	 * Sets to global log priority to the specified level
	 */
	SDL_LogPriority log_priority;

	/**
	 * --video-driver wayland/x11/...
	 *
	 * Override the default video driver with the specified one
	 */
	const char *video_driver;

	/**
	 * --allow-screensaver / --no-allow-screensaver
	 *
	 * Set the "allow screensaver" hint
	 */
	arg_option_t allow_screensaver;

	/**
	 * --fatal-error-message-box / --no-fatal-error-message-box
	 *
	 * Show a message box on fatal error
	 */
	arg_option_t fatal_error_message_box;

	/**
	 * --threads [0-]
	 *
	 * Enable ECS threads and set number of worker threads
	 */
	Sint32 threads;

	/**
	 * --task-threads [0-]
	 *
	 * Enable ECS task threads and set number of worker task threads
	 */
	Sint32 task_threads;
} args_t;

[[nodiscard]]
bool args_parse(int argc, char **argv, args_t *args);

/**
 * Format enabled as "1" and disabled as "0"
 */
[[nodiscard]]
const char *arg_option_str(arg_option_t option);
