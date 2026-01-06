#include "assets.h"
#include "image.h"

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

SDL_Surface *assets_load_texture(assets_t *assets, const char *name)
{
	char *path = nullptr;
	SDL_asprintf(&path, "textures/%s", name);

	SDL_IOStream *stream = assets_load(assets, path);
	SDL_Surface *surface = load_qoi(stream, true);

	SDL_free(path);
	return surface;
}
