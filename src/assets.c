#include "assets.h"

#include <SDL3/SDL_stdinc.h>

void assets_destroy(assets_t *assets)
{
	if (assets == nullptr)
	{
		return;
	}

	assets->cleanup(assets);
	SDL_free(assets);
}

SDL_IOStream *assets_load(assets_t *assets, const char *name)
{
	return assets->load(assets, name);
}
