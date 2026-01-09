#pragma once

#include "vector.h"

#include <SDL3/SDL_stdinc.h>

typedef struct physics_engine_t physics_engine_t;

typedef enum object_layer_t: Uint32
{
	OBJ_LAYER_DEFAULT = 0,
	OBJ_LAYER_STATIC  = 1,
	OBJ_LAYER_PLAYER  = 2,

	OBJ_LAYER_COUNT = 3,
} object_layer_t;

typedef enum physics_motion_type_t
{
	MOTION_TYPE_STATIC    = 0, // Non-movable
	MOTION_TYPE_KINEMATIC = 1, // Only movable using velocities
	MOTION_TYPE_DYNAMIC   = 2, // Normal physics object
} physics_motion_type_t;

typedef struct box_config_t
{
	physics_motion_type_t motion_type;
	object_layer_t layer;
	vector3f_t position;
	vector3f_t half_extents;
	bool activate;
} box_config_t;

typedef struct sphere_config_t
{
	physics_motion_type_t motion_type;
	object_layer_t layer;
	vector3f_t position;
	float radius;
	bool activate;
} sphere_config_t;

typedef struct capsule_config_t
{
	float half_height;
	float radius;
	vector3f_t position;
	physics_motion_type_t motion_type;
	object_layer_t layer;
	bool activate;
} capsule_config_t;

[[nodiscard]]
physics_engine_t *physics_engine_create();

void physics_engine_destroy(physics_engine_t *engine);

void physics_engine_optimize(const physics_engine_t *engine);

bool physics_engine_update(const physics_engine_t *engine, float delta);

void physics_engine_add_box(physics_engine_t *engine, const box_config_t *config);

void physics_engine_add_sphere(physics_engine_t *engine, const sphere_config_t *config);

void physics_engine_add_capsule(physics_engine_t *engine, const capsule_config_t *config);
