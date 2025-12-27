#include "image.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_surface.h>

#define QOI_MALLOC(sz) SDL_malloc(sz)
#define QOI_FREE(p)    SDL_free(p)
#define QOI_ZEROARR(a) SDL_zeroa(a)
#define QOI_NO_STDIO

#define QOI_IMPLEMENTATION
#include "qoi.h"

SDL_Surface *load_qoi(const char *path)
{
	size_t size = 0;
	void *data = SDL_LoadFile(path, &size);

	qoi_desc desc;
	void *pixels = qoi_decode(data, size, &desc, 4);
	if (pixels == nullptr)
	{
		SDL_SetError("Failed to decode image data");
		SDL_free(data);
		return nullptr;
	}
	SDL_free(data);

	SDL_Surface *surface = SDL_CreateSurfaceFrom((int) desc.width, (int) desc.height,
		SDL_PIXELFORMAT_RGBA32, pixels, (int) (desc.width * desc.channels));

	SDL_free(pixels);
	return surface;
}
