#include "assets.h"
#include "image.h"
#include "model.h"

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_surface.h>

SDL_Surface *assets_load_texture(const assets_t *assets, const char *name)
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

model_t *assets_load_model(const assets_t *assets, SDL_GPUDevice *device, const char *name)
{
	char *path = nullptr;
	if (SDL_asprintf(&path, "models/%s", name) < 0)
	{
		return nullptr;
	}

	SDL_IOStream *stream = assets_load(assets, path);
	SDL_free(path);

	if (stream == nullptr)
	{
		return nullptr;
	}

	return model_create(device, stream, true);
}

SDL_IOStream *assets_load_script(const assets_t *assets, const char *name)
{
	char *path = nullptr;
	if (SDL_asprintf(&path, "scripts/%s", name) < 0)
	{
		return nullptr;
	}

	SDL_IOStream *stream = assets_load(assets, path);
	SDL_free(path);

	return stream;
}
