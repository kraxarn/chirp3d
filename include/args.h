#pragma once

typedef struct
{
	/**
	 * --prefer-low-power / --no-prefer-low-power
	 *
	 * Create the GPU device with the "prefer low power" hint
	 */
	bool prefer_low_power;
} args_t;

[[nodiscard]]
args_t args_parse(int argc, char **argv);
