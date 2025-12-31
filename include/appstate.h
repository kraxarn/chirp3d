#pragma once

#include "camera.h"
#include "font.h"
#include "mesh.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

typedef struct app_state_t
{
	SDL_Window *window;
	SDL_GPUDevice *device;
	SDL_GPUGraphicsPipeline *pipeline;
	font_t *font;
	mesh_t *mesh;
	camera_t camera;
	Uint64 last_update;
	float dt;
	float current_rotation;
} app_state_t;
