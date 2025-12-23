#pragma once

#include "camera.h"
#include "mesh.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

typedef struct app_state_t
{
	SDL_Window *window;
	SDL_GPUDevice *device;
	SDL_GPUGraphicsPipeline *pipeline;
	mesh_t *mesh;
	float dt;
	camera_t camera;
} app_state_t;
