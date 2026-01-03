#pragma once

typedef struct assets_t assets_t;

typedef void *(*assets_load_func_t)(assets_t *assets, const char *name);

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
