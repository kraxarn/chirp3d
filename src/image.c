#include "image.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_surface.h>

#define QOI_MALLOC(sz) SDL_malloc(sz)
#define QOI_FREE(p)    SDL_free(p)
#define QOI_ZEROARR(a) SDL_zeroa(a)
#define QOI_NO_STDIO

#define QOI_IMPLEMENTATION
#include "qoi.h"

static constexpr auto channels = 4;

SDL_Surface *load_qoi(SDL_IOStream *source)
{
	size_t size = 0;
	void *data = SDL_LoadFile_IO(source, &size, true);

	qoi_desc desc;
	void *pixels = qoi_decode(data, (int) size, &desc, channels);
	SDL_free(data);

	if (pixels == nullptr)
	{
		SDL_SetError("Failed to decode image data");
		return nullptr;
	}

	SDL_Surface *surface = SDL_CreateSurfaceFrom((int) desc.width, (int) desc.height,
		SDL_PIXELFORMAT_RGBA32, pixels, (int) desc.width * channels);

	surface->flags &= SDL_SURFACE_PREALLOCATED;
	return surface;
}
