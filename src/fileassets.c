#include "assets.h"
#include "assetstream.h"
#include "input.h"
#include "inputconfig.h"
#include "json.h"
#include "logcategory.h"
#include "map.h"
#include "mousebutton.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_log.h>
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

[[nodiscard]]
static bool is_key(const char *json, const json_token_t *token, const char *key)
{
	return (bool) (token->type == JSON_STRING
		&& SDL_strncmp(json + token->start, key, token->end - token->start) == 0);
}

#define token_str(token) ((int) ((token)->end - (token)->start)), (json + (token)->start)

static bool parse_project(SDL_IOStream *stream, assets_t *assets)
{
	size_t json_len = 0;
	char *json = SDL_LoadFile_IO(stream, &json_len, true);

	json_parser_t parser;
	json_init(&parser);

	constexpr size_t token_count = 256;
	json_token_t tokens[token_count];

	const int count = json_parse(&parser, json, json_len, tokens, token_count);

	if (count == JSON_ERROR_OOM)
	{
		SDL_free(json);
		return SDL_SetError("Out of memory");
	}

	if (count == JSON_ERROR_INVALID
		|| count == JSON_ERROR_INCOMPLETE)
	{
		SDL_free(json);
		return SDL_SetError("Invalid JSON");
	}

	SDL_LogDebug(LOG_CATEGORY_ASSETS, "Parsed %d tokens", count);

	for (int i = 0; i < count; i++)
	{
		const json_token_t *token = tokens + i;

		const json_type_t type = i + 1 == count ? JSON_UNDEFINED : (token + 1)->type;
		const int size = i + 1 == count ? 0 : (token + 1)->size;

		if (is_key(json, token, "met") && type == JSON_OBJECT)
		{
			for (int j = 0; j < size; j++)
			{
				const json_token_t *key = token + 2 + (ptrdiff_t) (j * 2);
				const json_token_t *value = key + 1;

				const char *prop_name;
				if (is_key(json, key, "nam"))
				{
					prop_name = SDL_PROP_APP_METADATA_NAME_STRING;
				}
				else if (is_key(json, key, "ver"))
				{
					prop_name = SDL_PROP_APP_METADATA_VERSION_STRING;
				}
				else if (is_key(json, key, "ide"))
				{
					prop_name = SDL_PROP_APP_METADATA_IDENTIFIER_STRING;
				}
				else if (is_key(json, key, "cre"))
				{
					prop_name = SDL_PROP_APP_METADATA_CREATOR_STRING;
				}
				else if (is_key(json, key, "cop"))
				{
					prop_name = SDL_PROP_APP_METADATA_COPYRIGHT_STRING;
				}
				else if (is_key(json, key, "url"))
				{
					prop_name = SDL_PROP_APP_METADATA_URL_STRING;
				}
				else if (is_key(json, key, "typ"))
				{
					prop_name = SDL_PROP_APP_METADATA_TYPE_STRING;
				}
				else
				{
					SDL_SetError("Unknown metadata key: %.*s", token_str(key));
					SDL_free(json);
					return false;
				}

				json[value->end] = '\0';
				SDL_SetAppMetadataProperty(prop_name, json + value->start);
			}

			i += size;
			continue;
		}

		if (is_key(json, token, "win") && type == JSON_OBJECT)
		{
			for (int j = i + 2; j < count; j++)
			{
				const json_token_t *key = tokens + j;

				if (key->start < (token + 1)->start || key->end > (token + 1)->end)
				{
					break;
				}

				const json_token_t *value = key + 1;

				if (is_key(json, key, "tit"))
				{
					const size_t title_size = value->end - value->start + 1;
					char *title = SDL_malloc(title_size);
					SDL_strlcpy(title, json + value->start, title_size);

					assets->window_config.title = title;
				}
				else if (is_key(json, key, "siz"))
				{
					const long width = SDL_strtol(json + (value + 1)->start, nullptr, 10);
					const long height = SDL_strtol(json + (value + 2)->start, nullptr, 10);

					assets->window_config.size.x = (int) width;
					assets->window_config.size.y = (int) height;
				}
				else if (is_key(json, key, "ful"))
				{
					const bool fullscreen = SDL_strncmp(json + value->start, "true", 4) == 0;

					assets->window_config.fullscreen = fullscreen;
				}
				else if (is_key(json, key, "ico"))
				{
					// TODO: Icons are currently not supported
				}
				else
				{
					SDL_SetError("Unknown window key: %.*s", token_str(key));
					SDL_free(json);
					return false;
				}

				j += value->type == JSON_ARRAY ? 3 : 1;
			}

			i += size;
			continue;
		}

		if (is_key(json, token, "inp") && type == JSON_OBJECT)
		{
			const json_token_t *prev_parent = nullptr;
			input_config_t input_config = {0};

			for (int j = i + 2; j < count; j++)
			{
				const json_token_t *key = tokens + j;

				if (key->start < (token + 1)->start || key->end > (token + 1)->end)
				{
					break;
				}

				if ((key + 1)->type == JSON_OBJECT)
				{
					j++;
					continue;
				}

				const json_token_t *parent = tokens + key->parent - 1;
				const json_token_t *value = key + 1;

				if (prev_parent != nullptr && parent != prev_parent)
				{
					json[prev_parent->end] = '\0';
					if (!input_add(json + prev_parent->start, input_config))
					{
						SDL_free(json);
						return false;
					}
					SDL_zero(input_config);
				}

				if (is_key(json, key, "key"))
				{
					json[value->end] = '\0';
					input_config.keycode = SDL_GetKeyFromName(json + value->start);
					if (input_config.keycode == SDLK_UNKNOWN)
					{
						SDL_SetError("Unknown keycode for %.*s: %s",
							token_str(key), json + value->start);
						SDL_free(json);
						return false;
					}
				}
				else if (is_key(json, key, "mou"))
				{
					json[value->end] = '\0';
					input_config.mouse_button = mouse_button_from_name(json + value->start);
					if (input_config.mouse_button == 0)
					{
						SDL_SetError("Unknown mouse button for %.*s: %s",
							token_str(key), json + value->start);
						SDL_free(json);
						return false;
					}
				}
				else if (is_key(json, key, "axi"))
				{
					// TODO: Gamepads are currently not supported
				}
				else if (is_key(json, key, "ara"))
				{
					// TODO: Gamepads are currently not supported
				}
				else
				{
					SDL_SetError("Unknown input key: %.*s", token_str(key));
					SDL_free(json);
					return false;
				}

				j += value->type == JSON_ARRAY ? 3 : 1;
				prev_parent = parent;
			}

			if (prev_parent != nullptr)
			{
				json[prev_parent->end] = '\0';
				if (!input_add(json + prev_parent->start, input_config))
				{
					SDL_free(json);
					return false;
				}
			}

			i += size;
			continue;
		}
	}

	SDL_free(json);
	return true;
}

#undef token_str

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
	SDL_free((void*) assets->window_config.title);

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

	if (!parse_project(load(assets, "project"), assets))
	{
		SDL_CloseIO(stream);
		cleanup(assets);
		return nullptr;
	}

	return assets;
}
