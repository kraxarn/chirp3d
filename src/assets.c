#include "assets.h"

#include <SDL3/SDL_stdinc.h>

void assets_destroy(assets_t *assets)
{
	assets->cleanup(assets);
	SDL_free(assets);
}
