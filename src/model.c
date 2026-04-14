#include "model.h"
#include "logcategory.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

#define CGLTF_MALLOC(size) SDL_malloc(size)
#define CGLTF_FREE(ptr)    SDL_free(ptr)
#define CGLTF_ATOI(str)    SDL_atoi(str)
#define CGLTF_ATOF(str)    SDL_atof(str)
#define CGLTF_ATOLL(str)   SDL_strtoll(str,nullptr,10)

#ifndef NDEBUG
#define CGLTF_VALIDATE_ENABLE_ASSERTS 1
#endif

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

typedef struct model_t
{
	cgltf_data *data;
} model_t;

static const char *cgltf_error_string(const cgltf_result result)
{
	switch (result)
	{
		case cgltf_result_data_too_short: return "Data too short";
		case cgltf_result_unknown_format: return "Unknown format";
		case cgltf_result_invalid_json: return "Invalid JSON";
		case cgltf_result_invalid_gltf: return "Invalid glTF";
		case cgltf_result_invalid_options: return "Invalid options";
		case cgltf_result_file_not_found: return "File not found";
		case cgltf_result_io_error: return "I/O error";
		case cgltf_result_out_of_memory: return "Out of memory";
		case cgltf_result_legacy_gltf: return "Legacy glTF";
		default: return "Unknown error";
	}
}

static size_t append_debug_info(char *str, const size_t str_len,
	const char *key, const cgltf_size value)
{
	if (value <= 0)
	{
		return 0;
	}

	constexpr size_t temp_len = 32;
	char temp[temp_len];
	if (SDL_snprintf(temp, temp_len, ", %s: %zu", key, value) < 0)
	{
		return 0;
	}

	return SDL_strlcat(str, temp, str_len);
}

static void log_debug_info(const cgltf_data *data)
{
	char type_str[5];

	switch (data->file_type)
	{
		case cgltf_file_type_gltf:
			SDL_strlcpy(type_str, "gltf", sizeof(type_str));
			break;

		case cgltf_file_type_glb:
			SDL_strlcpy(type_str, "glb", sizeof(type_str));
			break;

		default:
			SDL_LogWarn(LOG_CATEGORY_MODEL, "Unknown model format");
			return;
	}

	constexpr size_t model_info_len = 256;
	char model_info[model_info_len];
	SDL_zeroa(model_info);

	append_debug_info(model_info, model_info_len, "meshes", data->meshes_count);
	append_debug_info(model_info, model_info_len, "materials", data->materials_count);
	append_debug_info(model_info, model_info_len, "accessors", data->accessors_count);
	append_debug_info(model_info, model_info_len, "buffer views", data->buffer_views_count);
	append_debug_info(model_info, model_info_len, "buffers", data->buffers_count);
	append_debug_info(model_info, model_info_len, "images", data->images_count);
	append_debug_info(model_info, model_info_len, "textures", data->textures_count);
	append_debug_info(model_info, model_info_len, "samplers", data->samplers_count);
	append_debug_info(model_info, model_info_len, "skins", data->skins_count);
	append_debug_info(model_info, model_info_len, "cameras", data->cameras_count);
	append_debug_info(model_info, model_info_len, "lights", data->lights_count);
	append_debug_info(model_info, model_info_len, "nodes", data->nodes_count);
	append_debug_info(model_info, model_info_len, "scenes", data->scenes_count);
	append_debug_info(model_info, model_info_len, "animations", data->animations_count);
	append_debug_info(model_info, model_info_len, "variants", data->variants_count);
	append_debug_info(model_info, model_info_len, "extensions", data->data_extensions_count);
	append_debug_info(model_info, model_info_len, "extensions used", data->extensions_used_count);
	append_debug_info(model_info, model_info_len, "extensions required", data->extensions_required_count);

	SDL_LogDebug(LOG_CATEGORY_MODEL, "Loaded model (type: %s%s)", type_str, model_info);
}

model_t *model_create(SDL_IOStream *stream, const bool close_io)
{
	cgltf_size file_size;
	void *file_data = SDL_LoadFile_IO(stream, &file_size, close_io);
	if (file_data == nullptr)
	{
		return nullptr;
	}

	model_t *model = SDL_malloc(sizeof(model_t));
	if (model == nullptr)
	{
		return nullptr;
	}

	const cgltf_options options = {};

	const cgltf_result result = cgltf_parse(&options, file_data, file_size, &model->data);
	SDL_free(file_data);

	if (result != cgltf_result_success)
	{
		SDL_SetError("%s", cgltf_error_string(result));
		SDL_free(model);
		return nullptr;
	}

	log_debug_info(model->data);

	return model;
}

void model_destroy(model_t *model)
{
	if (model == nullptr)
	{
		return;
	}

	cgltf_free(model->data);
	SDL_free(model);
}
