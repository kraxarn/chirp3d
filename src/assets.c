#include "assets.h"
#include "image.h"

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_surface.h>

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
	if (SDL_asprintf(&path, "textures/%s", name) < 0)
	{
		return nullptr;
	}

	SDL_IOStream *stream = assets_load(assets, path);
	SDL_free(path);

	if (stream == nullptr)
	{
		return nullptr;
	}

	return load_qoi(stream, true);
}
