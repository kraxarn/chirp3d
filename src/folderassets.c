#include "assets.h"
#include "logcategory.h"

#include "tomlc17.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

typedef struct folder_assets_t
{
	char *basepath;
	toml_result_t toml;
} folder_assets_t;

static bool set_metadata_property(const toml_datum_t table, const char *key)
{
	const toml_datum_t value = toml_get(table, key);
	if (value.type != TOML_STRING)
	{
		return false;
	}

	char *name = nullptr;
	if (SDL_asprintf(&name, "SDL.app.metadata.%s", key) < 0)
	{
		return false;
	}

	if (!SDL_SetAppMetadataProperty(name, value.u.s))
	{
		SDL_free(name);
		return false;
	}

	SDL_free(name);
	return true;
}

static SDL_IOStream *load(assets_t *assets, const char *path)
{
	const folder_assets_t *folder_assets = assets->data;

	const char *filename = SDL_strrchr(path, '/');
	if (filename == nullptr)
	{
		SDL_SetError("Invalid path");
		return nullptr;
	}
	filename++;

	const long parent_len = filename - path;
	char *parent = SDL_malloc(parent_len);
	SDL_strlcpy(parent, path, parent_len);

	char *dir_path = nullptr;
	SDL_asprintf(&dir_path, "%s/%s", folder_assets->basepath, parent);

	char *pattern = nullptr;
	SDL_asprintf(&pattern, "%s.*", filename);

	auto count = 0;
	char **results = SDL_GlobDirectory(dir_path, pattern, 0, &count);
	SDL_free(pattern);

	if (count == 0)
	{
		SDL_SetError("No asset found for '%s'", path);
		SDL_free((void *) results);
		SDL_free(dir_path);
		SDL_free(parent);
		return nullptr;
	}

	if (count > 1)
	{
		SDL_SetError("Multiple assets found for '%s'", path);
		SDL_free((void *) results);
		SDL_free(dir_path);
		SDL_free(parent);
		return nullptr;
	}

	SDL_LogDebug(LOG_CATEGORY_ASSETS, "Loaded asset '%s/%s' from '%s'",
		parent, results[0], path);
	SDL_free(parent);

	char *full_path = nullptr;
	SDL_asprintf(&full_path, "%s/%s", dir_path, results[0]);
	SDL_free(dir_path);
	SDL_free((void *) results);

	SDL_IOStream *stream = SDL_IOFromFile(full_path, "rb");
	SDL_free(full_path);

	return stream;
}

static void cleanup(assets_t *assets)
{
	folder_assets_t *data = assets->data;

	toml_free(data->toml);
	SDL_free(data->basepath);
	SDL_free(data);
}

static window_config_t parse_window_config(const toml_datum_t project)
{
	window_config_t config = window_config_default();

	const toml_datum_t window = toml_get(project, "window");

	const toml_datum_t toml_title = toml_get(window, "title");
	if (toml_title.type == TOML_STRING)
	{
		config.title = toml_title.u.s;
	}

	const toml_datum_t toml_size = toml_get(window, "size");
	if (toml_size.type == TOML_ARRAY
		&& toml_size.u.arr.size == 2
		&& toml_size.u.arr.elem[0].type == TOML_INT64
		&& toml_size.u.arr.elem[1].type == TOML_INT64)
	{
		config.size.x = (int) toml_size.u.arr.elem[0].u.int64;
		config.size.y = (int) toml_size.u.arr.elem[1].u.int64;
	}

	const toml_datum_t toml_fullscreen = toml_get(window, "fullscreen");
	if (toml_size.type == TOML_BOOLEAN)
	{
		config.fullscreen = toml_fullscreen.u.boolean;
	}

	return config;
}

assets_t *assets_create_from_folder(const char *path)
{
	char *toml_path = nullptr;
	SDL_asprintf(&toml_path, "%s/project.toml", path);

	// TODO: Is this really necessary?
	SDL_PathInfo toml_path_info;
	if (!SDL_GetPathInfo(toml_path, &toml_path_info) || toml_path_info.type != SDL_PATHTYPE_FILE)
	{
		SDL_free(toml_path);
		SDL_SetError("'project.toml' file missing or invalid");
		return nullptr;
	}

	size_t toml_size = 0;
	void *toml_data = SDL_LoadFile(toml_path, &toml_size);
	SDL_free(toml_path);

	if (toml_data == nullptr)
	{
		return nullptr;
	}

	const toml_result_t toml_result = toml_parse(toml_data, (int) toml_size);
	SDL_free(toml_data);

	if (!toml_result.ok)
	{
		SDL_SetError("Failed to parse project file: %s", toml_result.errmsg);
		return nullptr;
	}

	assets_t *assets = SDL_malloc(sizeof(assets_t));
	if (assets == nullptr)
	{
		toml_free(toml_result);
		return nullptr;
	}

	folder_assets_t *folder_assets = SDL_malloc(sizeof(folder_assets_t));
	if (folder_assets == nullptr)
	{
		toml_free(toml_result);
		return nullptr;
	}
	assets->data = folder_assets;

	assets->load = load;
	assets->cleanup = cleanup;

	folder_assets->toml = toml_result;

	folder_assets->basepath = nullptr;
	SDL_asprintf(&folder_assets->basepath, "%s/assets", path);

	const toml_datum_t table = toml_result.toptab;

	const toml_datum_t project = toml_get(table, "project");
	if (project.type != TOML_TABLE)
	{
		SDL_SetError("Invalid project node");
		SDL_free(assets);
		toml_free(toml_result);
		return nullptr;
	}

	assets->window_config = parse_window_config(project);

	const toml_datum_t metadata = toml_get(project, "metadata");
	if (metadata.type == TOML_TABLE)
	{
		set_metadata_property(metadata, "name");
		set_metadata_property(metadata, "version");
		set_metadata_property(metadata, "identifier");
		set_metadata_property(metadata, "creator");
		set_metadata_property(metadata, "copyright");
		set_metadata_property(metadata, "url");
		set_metadata_property(metadata, "type");
	}

	return assets;
}
