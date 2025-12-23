#pragma once

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

SDL_GPUDevice *create_device(SDL_Window *window);

SDL_GPUGraphicsPipeline *create_pipeline(SDL_GPUDevice *device, SDL_Window *window,
	SDL_GPUShader *vertex_shader, SDL_GPUShader *fragment_shader);
