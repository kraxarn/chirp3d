#include "tests.h"

#include <SDL3/SDL_stdinc.h>

int main(const int argc, char **argv)
{
	if (argc < 2)
	{
		return 1;
	}

	switch (SDL_strtol(argv[1], nullptr, 10))
	{
		case 1:
			test_array();
			return 0;

		default:
			return 1;
	}
}
