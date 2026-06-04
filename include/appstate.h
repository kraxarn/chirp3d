#pragma once

#include "assets.h"
#include "camera.h"
#include "ecs.h"
#include "model.h"
#include "physics.h"
#include "physicsconfig.h"

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>

typedef struct
{
	Uint16 fps;
	Uint16 count;
	float duration;
} time_info_t;

typedef struct
{
	node_instance_t *instance;
	physics_body_id_t body_id;
} node_instance_physics_t;

typedef struct
{
	SDL_Window *window;
	SDL_GPUDevice *device;
	SDL_GPUGraphicsPipeline *pipeline;
	SDL_GPUTexture *depth_texture;

	assets_t *assets;
	camera_t camera;
	physics_engine_t *physics_engine;
	physics_config_t physics_config;
	ecs_world_t *ecs;

	Uint64 last_update;
	time_info_t time;
	float dt;

	model_t **models;
	node_instance_t **instances;

	physics_body_id_t player_body_id;
	node_instance_physics_t *instance_physics;
} app_state_t;
