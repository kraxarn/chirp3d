#pragma once

#include "mesh.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

SDL_GPUDevice *create_device(SDL_Window *window);

SDL_GPUGraphicsPipeline *create_pipeline(SDL_GPUDevice *device, SDL_Window *window,
	SDL_GPUBuffer *vertex_buffer, SDL_GPUBuffer *index_buffer);

bool create_mesh_buffers(SDL_GPUDevice *device, mesh_t mesh,
	SDL_GPUBuffer **vertex_buffer, SDL_GPUBuffer **index_buffer);
