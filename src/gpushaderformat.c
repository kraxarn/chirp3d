#include "gpushaderformat.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>

char *shader_format_names(SDL_GPUDevice *device)
{
	// "invalid, private, spir-v, dxbc, dxil, msl, metallib"
	constexpr size_t shader_format_names_len = 51;

	char *shader_format_names = SDL_malloc(sizeof(char) * (shader_format_names_len + 1));
	if (shader_format_names == nullptr)
	{
		return nullptr;
	}
	shader_format_names[0] = '\0';

	const SDL_GPUShaderFormat shader_format = SDL_GetGPUShaderFormats(device);

	if (shader_format == SDL_GPU_SHADERFORMAT_INVALID)
	{
		SDL_strlcat(shader_format_names, "invalid, ", shader_format_names_len);
	}
	if ((shader_format & SDL_GPU_SHADERFORMAT_PRIVATE) > 0)
	{
		SDL_strlcat(shader_format_names, "private, ", shader_format_names_len);
	}
	if ((shader_format & SDL_GPU_SHADERFORMAT_SPIRV) > 0)
	{
		SDL_strlcat(shader_format_names, "spir-v, ", shader_format_names_len);
	}
	if ((shader_format & SDL_GPU_SHADERFORMAT_DXBC) > 0)
	{
		SDL_strlcat(shader_format_names, "dxbc, ", shader_format_names_len);
	}
	if ((shader_format & SDL_GPU_SHADERFORMAT_DXIL) > 0)
	{
		SDL_strlcat(shader_format_names, "dxil, ", shader_format_names_len);
	}
	if ((shader_format & SDL_GPU_SHADERFORMAT_MSL) > 0)
	{
		SDL_strlcat(shader_format_names, "msl, ", shader_format_names_len);
	}
	if ((shader_format & SDL_GPU_SHADERFORMAT_METALLIB) > 0)
	{
		SDL_strlcat(shader_format_names, "metallib, ", shader_format_names_len);
	}

	shader_format_names[SDL_strlen(shader_format_names) - 2] = '\0';
	return shader_format_names;
}
