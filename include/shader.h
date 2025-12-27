#pragma once

#include <SDL3/SDL_gpu.h>

[[nodiscard]]
SDL_GPUShaderFormat shader_format(SDL_GPUDevice *device);

[[nodiscard]]
SDL_GPUShader *load_shader(SDL_GPUDevice *device, SDL_IOStream *source, SDL_GPUShaderStage stage,
	int num_samplers, int num_uniform_buffers);
