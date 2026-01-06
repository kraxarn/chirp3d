#pragma once

#include "windowconfig.h"

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_surface.h>

typedef struct assets_t assets_t;

typedef SDL_IOStream *(*assets_load_func_t)(assets_t *assets, const char *name);

typedef void (*assets_cleanup_func_t)(assets_t *assets);

typedef struct assets_t
{
	assets_load_func_t load;
	assets_cleanup_func_t cleanup;
	window_config_t window_config;
	void *data;
} assets_t;

[[nodiscard]]
assets_t *assets_create_from_folder(const char *path);

void assets_destroy(assets_t *assets);

[[nodiscard]]
SDL_IOStream *assets_load(assets_t *assets, const char *name);

[[nodiscard]]
SDL_Surface *assets_load_texture(assets_t *assets, const char *name);
