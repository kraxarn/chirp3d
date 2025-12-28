#pragma once

#include "camera.h"
#include "mesh.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

typedef struct app_state_t
{
	SDL_Window *window;
	SDL_GPUDevice *device;
	SDL_GPUGraphicsPipeline *pipeline;
	SDL_Renderer *renderer;
	mesh_t *mesh;
	camera_t camera;
	Uint64 last_update;
	float current_rotation;
} app_state_t;
