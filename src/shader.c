#include "shader.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_gpu.h>

SDL_GPUShaderFormat shader_format(SDL_GPUDevice *device)
{
	const SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device);

	if ((formats & SDL_GPU_SHADERFORMAT_MSL) > 0)
	{
		return SDL_GPU_SHADERFORMAT_MSL;
	}

	if ((formats & SDL_GPU_SHADERFORMAT_DXIL) > 0)
	{
		return SDL_GPU_SHADERFORMAT_DXIL;
	}

	if ((formats & SDL_GPU_SHADERFORMAT_SPIRV) > 0)
	{
		return SDL_GPU_SHADERFORMAT_SPIRV;
	}

	SDL_SetError("Unsupported shader formats: %d", formats);
	return SDL_GPU_SHADERFORMAT_INVALID;
}

[[nodiscard]]
static const char *entrypoint(const SDL_GPUShaderFormat shader_format)
{
	// For some reason, MSL shaders get a "main0" entrypoint (???)
	if (shader_format == SDL_GPU_SHADERFORMAT_MSL)
	{
		return "main0";
	}

	return "main";
}

SDL_GPUShader *load_shader(SDL_GPUDevice *device, const char *filename,
	const SDL_GPUShaderStage stage, const int num_uniform_buffers)
{
	char *path = nullptr;
	SDL_asprintf(&path, "%s%s", SDL_GetBasePath(), filename);

	size_t size = 0;
	Uint8 *data = SDL_LoadFile(path, &size);
	if (data == nullptr)
	{
		SDL_free(path);
		return nullptr;
	}
	SDL_free(path);

	const SDL_GPUShaderFormat format = shader_format(device);
	if (format == SDL_GPU_SHADERFORMAT_INVALID)
	{
		SDL_free(data);
		return nullptr;
	}

	const SDL_GPUShaderCreateInfo create_info = {
		.code_size = size,
		.code = data,
		.entrypoint = entrypoint(format),
		.format = format,
		.stage = stage,
		.num_samplers = 0,
		.num_storage_buffers = 0,
		.num_storage_textures = 0,
		.num_uniform_buffers = num_uniform_buffers,
	};

	SDL_GPUShader *shader = SDL_CreateGPUShader(device, &create_info);
	if (shader == nullptr)
	{
		SDL_free(data);
		return nullptr;
	}
	SDL_free(data);

	return shader;
}
