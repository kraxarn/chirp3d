#pragma once

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
} args_t;

[[nodiscard]]
args_t args_parse(int argc, char **argv);
