#pragma once

#include <SDL3/SDL_gpu.h>

[[nodiscard]]
SDL_GPUShader *load_shader(SDL_GPUDevice *device, const char *filename,
	SDL_GPUShaderStage stage, int num_uniform_buffers);
