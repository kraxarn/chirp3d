#pragma once

typedef struct assets_t assets_t;

[[nodiscard]]
assets_t *assets_create_from_folder(const char *path);

void assets_destroy(assets_t *assets);
