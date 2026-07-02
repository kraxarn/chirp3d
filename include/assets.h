#pragma once

#include "input.h"
#include "map.h"
#include "model.h"
#include "windowconfig.h"

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_surface.h>

typedef struct
{
	SDL_IOStream *stream;
	window_config_t window_config;
	map_t desc;
} assets_t;

bool assets_create(const char *path, assets_t *assets);

void assets_destroy(const assets_t *assets);

[[nodiscard]]
window_config_t assets_window_config(const assets_t *assets);

[[nodiscard]]
SDL_IOStream *assets_load(const assets_t *assets, const char *name);

[[nodiscard]]
SDL_Surface *assets_load_texture(const assets_t *assets, const char *name);

[[nodiscard]]
bool assets_load_model(const assets_t *assets, SDL_GPUDevice *device, const char *name, model_t *model);

[[nodiscard]]
SDL_IOStream *assets_load_script(const assets_t *assets, const char *name);
