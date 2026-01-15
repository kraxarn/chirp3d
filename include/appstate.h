#pragma once

#include "assets.h"
#include "camera.h"
#include "mesh.h"
#include "physics.h"
#include "physicsconfig.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>

typedef struct time_info_t
{
	Uint16 fps;
	Uint16 count;
	float duration;
} time_info_t;

typedef struct app_state_t
{
	SDL_Window *window;
	SDL_GPUDevice *device;
	SDL_GPUGraphicsPipeline *pipeline;
	SDL_GPUTexture *depth_texture;

	assets_t *assets;
	camera_t camera;
	physics_engine_t *physics_engine;
	physics_config_t physics_config;

	Uint64 last_update;
	time_info_t time;
	float dt;

	mesh_t **meshes;
	size_t num_meshes;
	physics_body_id_t player_body_id;
} app_state_t;
