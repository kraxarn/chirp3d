#include "assets.h"

#include "tomlc17.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_stdinc.h>

typedef struct folder_assets_t
{
	char *basepath;
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

static void *load(assets_t *assets, const char *path)
{
	const folder_assets_t *folder_assets = assets->data;

	char *full_path = nullptr;
	SDL_asprintf(&full_path, "%s/%s", folder_assets->basepath, path);

	void *data = SDL_LoadFile(full_path, nullptr);
	SDL_free(full_path);

	return data;
}

static void cleanup(assets_t *assets)
{
	folder_assets_t *data = assets->data;

	SDL_free(data->basepath);
	SDL_free(data);
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

	toml_free(toml_result);
	return assets;
}
