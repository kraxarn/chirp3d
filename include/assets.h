#pragma once

#include "model.h"
#include "windowconfig.h"

#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_surface.h>

typedef struct assets_t assets_t;

[[nodiscard]]
assets_t *assets_create(const char *path);

void assets_destroy(assets_t *assets);

[[nodiscard]]
window_config_t assets_window_config(const assets_t *assets);

[[nodiscard]]
SDL_IOStream *assets_load(const assets_t *assets, const char *name);

[[nodiscard]]
SDL_Surface *assets_load_texture(const assets_t *assets, const char *name);

[[nodiscard]]
model_t *assets_load_model(const assets_t *assets, SDL_GPUDevice *device, const char *name);

[[nodiscard]]
SDL_IOStream *assets_load_script(const assets_t *assets, const char *name);
