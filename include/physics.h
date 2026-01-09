#pragma once

#include "vector.h"

#include <SDL3/SDL_stdinc.h>

typedef struct physics_engine_t physics_engine_t;

typedef enum object_layer_t: Uint32
{
	OBJ_LAYER_NON_MOVING = 0,
	OBJ_LAYER_MOVING     = 1,

	OBJ_LAYER_COUNT = 2,
} object_layer_t;

typedef enum physics_motion_type_t
{
	MOTION_TYPE_STATIC    = 0,
	MOTION_TYPE_KINEMATIC = 1,
	MOTION_TYPE_DYNAMIC   = 2,
} physics_motion_type_t;

[[nodiscard]]
physics_engine_t *physics_engine_create();

void physics_engine_destroy(physics_engine_t *engine);

void physics_engine_optimize(const physics_engine_t *engine);

void physics_engine_add_box(physics_engine_t *engine, physics_motion_type_t motion_type,
	object_layer_t layer, vector3f_t position, vector3f_t half_extents);

void physics_engine_add_sphere(physics_engine_t *engine, physics_motion_type_t motion_type,
	object_layer_t layer, vector3f_t position, float radius);
