#pragma once

#include <SDL3/SDL_iostream.h>

typedef struct assets_t assets_t;

typedef SDL_IOStream *(*assets_load_func_t)(assets_t *assets, const char *name);

typedef void (*assets_cleanup_func_t)(assets_t *assets);

typedef struct assets_t
{
	assets_load_func_t load;
	assets_cleanup_func_t cleanup;
	void *data;
} assets_t;

[[nodiscard]]
assets_t *assets_create_from_folder(const char *path);

void assets_destroy(assets_t *assets);

[[nodiscard]]
SDL_IOStream *assets_load(assets_t *assets, const char *name);
