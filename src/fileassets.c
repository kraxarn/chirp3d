#include "assets.h"
#include "assetstream.h"
#include "json.h"
#include "map.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>

typedef struct
{
	Uint16 flags;
	Uint32 offset;
	Uint32 size;
} file_descriptor_t;

typedef struct
{
	SDL_IOStream *stream;
	map_t desc;
} file_assets_t;

// Supported version of nest
static constexpr Uint8 nest_version = 1;

static SDL_IOStream *load(assets_t *assets, const char *path)
{
	const file_assets_t *data = assets->data;

	const size_t path_len = SDL_strlen(path);
	const Uint32 hash = SDL_murmur3_32(path, path_len, path_len);

	static constexpr size_t hash_str_len = 9;
	char hash_str[hash_str_len];
	SDL_snprintf(hash_str, hash_str_len, "%x", hash);

	const file_descriptor_t *desc = map_get(data->desc, hash_str, nullptr);
	if (desc == nullptr)
	{
		SDL_SetError("File not found: %s", path);
		return nullptr;
	}

	return asset_stream_open_io(data->stream, desc->offset, desc->size);
}

static void cleanup(assets_t *assets)
{
	file_assets_t *data = assets->data;
	map_destroy(data->desc);
	SDL_free(data);
}

[[nodiscard]]
static bool validate_header(SDL_IOStream *stream)
{
	char magic[5] = {0};
	if (!SDL_ReadU32LE(stream, (Uint32*) &magic))
	{
		return false;
	}

	if (SDL_strcmp(magic, "nest") != 0)
	{
		return SDL_SetError("Invalid nest file");
	}

	Uint8 version;
	if (!SDL_ReadU8(stream, &version))
	{
		return false;
	}

	if (version != nest_version)
	{
		return SDL_SetError("Unsupported nest version");
	}

	return true;
}

assets_t *assets_create(const char *path)
{
	SDL_IOStream *stream = SDL_IOFromFile(path, "rb");
	if (stream == nullptr)
	{
		return nullptr;
	}

	if (!validate_header(stream))
	{
		SDL_CloseIO(stream);
		return nullptr;
	}

	Uint32 file_count = 0;
	if (!SDL_ReadU32LE(stream, &file_count))
	{
		SDL_CloseIO(stream);
		return nullptr;
	}

	assets_t *assets = SDL_malloc(sizeof(assets_t));
	if (assets == nullptr)
	{
		SDL_CloseIO(stream);
		return nullptr;
	}

	file_assets_t *file_assets = SDL_malloc(sizeof(file_assets_t));
	if (file_assets == nullptr)
	{
		SDL_free(assets);
		SDL_CloseIO(stream);
		return nullptr;
	}

	file_assets->stream = stream;
	file_assets->desc = map_create();

	if (file_assets->desc == 0)
	{
		SDL_free(assets);
		SDL_free(file_assets);
		SDL_CloseIO(stream);
		return nullptr;
	}

	assets->data = file_assets;
	assets->load = load;
	assets->cleanup = cleanup;

	for (Uint32 i = 0; i < file_count; i++)
	{
		Uint32 hash;
		file_descriptor_t *descriptor = SDL_malloc(sizeof(file_descriptor_t));

		if (!SDL_ReadU32LE(stream, &hash)
			|| !SDL_ReadU16LE(stream, &descriptor->flags)
			|| !SDL_ReadU32LE(stream, &descriptor->offset)
			|| !SDL_ReadU32LE(stream, &descriptor->size))
		{
			SDL_CloseIO(stream);
			cleanup(assets);
			return nullptr;
		}

		if (descriptor->flags != 0)
		{
			SDL_SetError("Unknown flag: %d", descriptor->flags);
			SDL_CloseIO(stream);
			cleanup(assets);
			return nullptr;
		}

		static constexpr size_t hash_str_len = 9;
		char hash_str[hash_str_len];
		SDL_snprintf(hash_str, hash_str_len, "%x", hash);

		if (!map_set_with_cleanup(file_assets->desc, hash_str, descriptor, SDL_free))
		{
			SDL_CloseIO(stream);
			cleanup(assets);
			return nullptr;
		}
	}

	return assets;
}
