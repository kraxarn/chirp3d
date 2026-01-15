#include "model.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_iostream.h>
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

static const char *cgltf_error_string(const cgltf_result result)
{
	switch (result)
	{
		case cgltf_result_data_too_short: return "Data too short";
		case cgltf_result_unknown_format: return "Unknown format";
		case cgltf_result_invalid_json: return "Invalid JSON";
		case cgltf_result_invalid_gltf: return "Invalid GLTF";
		case cgltf_result_invalid_options: return "Invalid options";
		case cgltf_result_file_not_found: return "File not found";
		case cgltf_result_io_error: return "I/O error";
		case cgltf_result_out_of_memory: return "Out of memory";
		case cgltf_result_legacy_gltf: return "Legacy glTF";
		default: return "Unknown error";
	}
}

bool load_gltf(SDL_IOStream *stream, const bool close_io)
{
	cgltf_size file_size;
	void *file_data = SDL_LoadFile_IO(stream, &file_size, close_io);
	if (file_data == nullptr)
	{
		return false;
	}

	const cgltf_options options = {};

	cgltf_data *data = nullptr;
	const cgltf_result result = cgltf_parse(&options, file_data, file_size, &data);
	SDL_free(file_data);

	if (result != cgltf_result_success)
	{
		return SDL_SetError("%s", cgltf_error_string(result));
	}

	cgltf_free(data);
	return true;
}
