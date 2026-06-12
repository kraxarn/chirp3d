#pragma once

#include "vector.h"

#include <SDL3/SDL_stdinc.h>

typedef struct JPH_JobSystem JPH_JobSystem;
typedef struct JPH_PhysicsSystem JPH_PhysicsSystem;
typedef struct JPH_BodyInterface JPH_BodyInterface;
typedef Uint32 JPH_BodyID;

// Physics settings
static constexpr Uint32 max_bodies = SDL_MAX_UINT16;
static constexpr Uint32 num_body_mutexes = 0;
static constexpr Uint32 max_body_pairs = SDL_MAX_UINT16;
static constexpr Uint32 max_contact_constraints = SDL_MAX_UINT16;

typedef struct physics_engine_t
{
	JPH_JobSystem *job_system;
	JPH_PhysicsSystem *physics_system;
	JPH_BodyInterface *body_interface;

	JPH_BodyID bodies[max_bodies];
	size_t num_bodies;
} physics_engine_t;

typedef Uint32 physics_body_id_t;

typedef enum object_layer_t: Uint32
{
	OBJ_LAYER_DEFAULT = 0,
	OBJ_LAYER_STATIC  = 1,
	OBJ_LAYER_DYNAMIC = 2,

	OBJ_LAYER_COUNT = 3,
} object_layer_t;

typedef enum physics_motion_type_t
{
	MOTION_TYPE_STATIC    = 0, // Non-movable
	MOTION_TYPE_KINEMATIC = 1, // Only movable using velocities
	MOTION_TYPE_DYNAMIC   = 2, // Normal physics object
} physics_motion_type_t;

typedef enum physics_allowed_dof_t
{
	DOF_ALL           = 0b111111,
	DOF_TRANSLATION_X = 0b000001,
	DOF_TRANSLATION_Y = 0b000010,
	DOF_TRANSLATION_Z = 0b000100,
	DOF_ROTATION_X    = 0b001000,
	DOF_ROTATION_Y    = 0b010000,
	DOF_ROTATION_Z    = 0b100000,

	DOF_TRANSLATION_3D = DOF_TRANSLATION_X | DOF_TRANSLATION_Y | DOF_TRANSLATION_Z,
	DOF_ROTATION_3D    = DOF_ROTATION_X | DOF_ROTATION_Y | DOF_ROTATION_Z,
} physics_allowed_dof_t;

typedef struct
{
	vector3f_t position;
	physics_motion_type_t motion_type;
	object_layer_t layer;
	bool activate;
} body_config_t;

typedef struct
{
	vector3f_t half_extents;
	float friction;
	body_config_t body;
} box_config_t;

typedef struct
{
	float radius;
	body_config_t body;
} sphere_config_t;

typedef struct
{
	float half_height;
	float radius;
	physics_allowed_dof_t allowed_dof;
	float max_linear_velocity;
	body_config_t body;
} capsule_config_t;

typedef struct
{
	float half_height;
	float radius;
	body_config_t body;
} cylinder_config_t;

bool physics_create(physics_engine_t *engine);

void physics_destroy(const physics_engine_t *engine);

void physics_optimize(const physics_engine_t *engine);

bool physics_update(const physics_engine_t *engine, float delta);

void physics_set_gravity(const physics_engine_t *engine, vector3f_t gravity);

physics_body_id_t physics_add_box(physics_engine_t *engine, const box_config_t *config);

physics_body_id_t physics_add_sphere(physics_engine_t *engine, const sphere_config_t *config);

physics_body_id_t physics_add_capsule(physics_engine_t *engine, const capsule_config_t *config);

physics_body_id_t physics_add_cylinder(physics_engine_t *engine, const cylinder_config_t *config);

[[nodiscard]]
vector3f_t physics_body_position(const physics_engine_t *engine, physics_body_id_t body_id);

void physics_body_set_position(const physics_engine_t *engine, physics_body_id_t body_id,
	vector3f_t position, bool activate);

[[nodiscard]]
vector4f_t physics_body_rotation(const physics_engine_t *engine, physics_body_id_t body_id);

void physics_body_set_rotation(const physics_engine_t *engine,
	physics_body_id_t body_id, vector4f_t rotation, bool activate);

[[nodiscard]]
vector3f_t physics_body_linear_velocity(const physics_engine_t *engine, physics_body_id_t body_id);

void physics_body_set_linear_velocity(const physics_engine_t *engine,
	physics_body_id_t body_id, vector3f_t velocity);

void physics_body_add_linear_velocity(const physics_engine_t *engine,
	physics_body_id_t body_id, vector3f_t velocity);
