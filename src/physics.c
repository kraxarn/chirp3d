#include "physics.h"
#include "jphutils.h"
#include "logcategory.h"

#include "joltc.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

typedef enum broad_phase_layers_t: JPH_BroadPhaseLayer
{
	BP_LAYER_NON_MOVING = 0,
	BP_LAYER_MOVING     = 1,
	BP_LAYER_COUNT      = 2,
} broad_phase_layers_t;

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

static void on_trace(const char *message)
{
	SDL_LogInfo(LOG_CATEGORY_PHYSICS, "%s", message);
}

static bool on_assert(const char *expression, const char *message, const char *file, const uint32_t line)
{
	// TODO: I dunno if this works
	SDL_AssertData data = {
		.trigger_count = 1,
		.condition = message,
		.filename = file,
		.linenum = (int) line,
		.function = "JPH_TraceHandler",
		.next = nullptr,
	};
	SDL_ReportAssertion(&data, expression, file, (int) line);
	return true;
}

physics_engine_t *physics_create()
{
	if (!JPH_Init())
	{
		SDL_SetError("JPH initialisation failed");
		return nullptr;
	}

	JPH_SetTraceHandler(on_trace);
	JPH_SetAssertFailureHandler(on_assert);

	physics_engine_t *engine = SDL_malloc(sizeof(physics_engine_t));
	if (engine == nullptr)
	{
		return nullptr;
	}

	engine->num_bodies = 0;
	engine->job_system = JPH_JobSystemThreadPool_Create(nullptr);

	JPH_ObjectLayerPairFilter *obj_filter = JPH_ObjectLayerPairFilterTable_Create(OBJ_LAYER_COUNT);
	JPH_ObjectLayerPairFilterTable_EnableCollision(obj_filter,
		OBJ_LAYER_STATIC, OBJ_LAYER_PLAYER
	);
	JPH_ObjectLayerPairFilterTable_EnableCollision(obj_filter,
		OBJ_LAYER_PLAYER, OBJ_LAYER_STATIC
	);

	JPH_BroadPhaseLayerInterface *bp_interface = JPH_BroadPhaseLayerInterfaceTable_Create(
		OBJ_LAYER_COUNT, BP_LAYER_COUNT
	);
	JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer(bp_interface,
		OBJ_LAYER_STATIC, BP_LAYER_NON_MOVING
	);
	JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer(bp_interface,
		OBJ_LAYER_PLAYER, BP_LAYER_MOVING
	);

	JPH_ObjectVsBroadPhaseLayerFilter *layer_filter = JPH_ObjectVsBroadPhaseLayerFilterTable_Create(
		bp_interface, BP_LAYER_COUNT,
		obj_filter, OBJ_LAYER_COUNT
	);

	const JPH_PhysicsSystemSettings settings = {
		.maxBodies = max_bodies,
		.numBodyMutexes = num_body_mutexes,
		.maxBodyPairs = max_body_pairs,
		.maxContactConstraints = max_contact_constraints,
		.broadPhaseLayerInterface = bp_interface,
		.objectLayerPairFilter = obj_filter,
		.objectVsBroadPhaseLayerFilter = layer_filter,
	};
	engine->physics_system = JPH_PhysicsSystem_Create(&settings);

	engine->body_interface = JPH_PhysicsSystem_GetBodyInterface(engine->physics_system);

	return engine;
}

void physics_destroy(physics_engine_t *engine)
{
	if (engine == nullptr)
	{
		return;
	}

	for (size_t i = 0; i < engine->num_bodies; i++)
	{
		JPH_BodyInterface_RemoveAndDestroyBody(engine->body_interface, engine->bodies[i]);
	}

	JPH_JobSystem_Destroy(engine->job_system);
	JPH_PhysicsSystem_Destroy(engine->physics_system);
	JPH_Shutdown();

	SDL_free(engine);
}

void physics_optimize(const physics_engine_t *engine)
{
	const Uint64 start = SDL_GetTicks();

	JPH_PhysicsSystem_OptimizeBroadPhase(engine->physics_system);

	const Uint64 end = SDL_GetTicks();
	SDL_LogDebug(LOG_CATEGORY_PHYSICS, "Optimised broad phase in %llu ms", end - start);
}

bool physics_update(const physics_engine_t *engine, const float delta)
{
	constexpr auto collision_steps = 1;

	const JPH_PhysicsUpdateError error = JPH_PhysicsSystem_Update(engine->physics_system,
		delta, collision_steps, engine->job_system);

	switch (error)
	{
		case JPH_PhysicsUpdateError_None:
			return true;

		case JPH_PhysicsUpdateError_ManifoldCacheFull:
			return SDL_SetError("The manifest cache is full");

		case JPH_PhysicsUpdateError_BodyPairCacheFull:
			return SDL_SetError("The body pair cache is full");

		case JPH_PhysicsUpdateError_ContactConstraintsFull:
			return SDL_SetError("The contact constraints buffer is full");

		default:
			return SDL_SetError("Unknown error");
	}
}

void physics_set_gravity(const physics_engine_t *engine, const vector3f_t gravity)
{
	JPH_PhysicsSystem_SetGravity(engine->physics_system, jph_vec3(&gravity));
}

static JPH_BodyID add_body(physics_engine_t *engine, const JPH_BodyCreationSettings *settings, const bool activate)
{
	const JPH_Activation activation = jph_activation(activate);
	const JPH_BodyID body = JPH_BodyInterface_CreateAndAddBody(engine->body_interface, settings, activation);
	engine->bodies[engine->num_bodies++] = body;
	return body;
}

physics_body_id_t physics_add_box(physics_engine_t *engine, const box_config_t *config)
{
	JPH_BoxShape *shape = JPH_BoxShape_Create(
		jph_vec3(&config->half_extents), JPH_DEFAULT_CONVEX_RADIUS
	);

	JPH_BodyCreationSettings *settings = JPH_BodyCreationSettings_Create3(
		(JPH_Shape *) shape, jph_vec3(&config->position), nullptr,
		jph_motion_type(config->motion_type), config->layer
	);

	const JPH_BodyID body_id = add_body(engine, settings, config->activate);
	JPH_BodyCreationSettings_Destroy(settings);
	return body_id;
}

physics_body_id_t physics_add_sphere(physics_engine_t *engine, const sphere_config_t *config)
{
	JPH_SphereShape *shape = JPH_SphereShape_Create(config->radius);

	JPH_BodyCreationSettings *settings = JPH_BodyCreationSettings_Create3(
		(JPH_Shape *) shape, jph_vec3(&config->position), nullptr,
		jph_motion_type(config->motion_type), config->layer
	);

	const JPH_BodyID body_id = add_body(engine, settings, config->activate);
	JPH_BodyCreationSettings_Destroy(settings);
	return body_id;
}

physics_body_id_t physics_add_capsule(physics_engine_t *engine, const capsule_config_t *config)
{
	JPH_CapsuleShape *shape = JPH_CapsuleShape_Create(config->half_height, config->radius);

	JPH_BodyCreationSettings *settings = JPH_BodyCreationSettings_Create3(
		(JPH_Shape *) shape, jph_vec3(&config->position), nullptr,
		jph_motion_type(config->motion_type), config->layer
	);

	if (config->allowed_dof != 0)
	{
		JPH_BodyCreationSettings_SetAllowedDOFs(settings, jph_allowed_dof(config->allowed_dof));
	}

	const JPH_BodyID body_id = add_body(engine, settings, config->activate);
	JPH_BodyCreationSettings_Destroy(settings);
	return body_id;
}

vector3f_t physics_body_position(const physics_engine_t *engine, const physics_body_id_t body_id)
{
	vector3f_t position;
	JPH_BodyInterface_GetPosition(engine->body_interface, body_id, jph_vec3(&position));
	return position;
}

void physics_body_set_position(const physics_engine_t *engine, const physics_body_id_t body_id,
	vector3f_t position, const bool activate)
{
	JPH_BodyInterface_SetPosition(engine->body_interface, body_id, jph_vec3(&position), jph_activation(activate));
}

vector4f_t physics_body_rotation(const physics_engine_t *engine, const physics_body_id_t body_id)
{
	vector4f_t rotation;
	JPH_BodyInterface_GetRotation(engine->body_interface, body_id, jph_quat(&rotation));
	return rotation;
}

vector3f_t physics_body_linear_velocity(const physics_engine_t *engine, const physics_body_id_t body_id)
{
	vector3f_t velocity;
	JPH_BodyInterface_GetLinearVelocity(engine->body_interface, body_id, jph_vec3(&velocity));
	return velocity;
}

void physics_body_add_linear_velocity(const physics_engine_t *engine,
	const physics_body_id_t body_id, const vector3f_t velocity)
{
	JPH_BodyInterface_AddLinearVelocity(engine->body_interface, body_id, jph_vec3(&velocity));
}
