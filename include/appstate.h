#pragma once

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_video.h>

typedef struct app_state_t
{
	SDL_Window *window;
	SDL_GPUDevice *device;
} app_state_t;
