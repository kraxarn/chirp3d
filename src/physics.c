#include "physics.h"
#include "logcategory.h"
#include "vector.h"

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

	JPH_BodyID bodies[max_bodies]; // TODO: Is this a bit excessive maybe?
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

physics_engine_t *physics_engine_create()
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

void physics_engine_destroy(physics_engine_t *engine)
{
	if (engine == nullptr)
	{
		return;
	}

	JPH_JobSystem_Destroy(engine->job_system);
	JPH_PhysicsSystem_Destroy(engine->physics_system);
	JPH_Shutdown();

	SDL_free(engine);
}

void physics_engine_optimize(const physics_engine_t *engine)
{
	const Uint64 start = SDL_GetTicks();

	JPH_PhysicsSystem_OptimizeBroadPhase(engine->physics_system);

	const Uint64 end = SDL_GetTicks();
	SDL_LogDebug(LOG_CATEGORY_PHYSICS, "Optimised broad phase in %llu ms", end - start);
}

static JPH_Vec3 jph_vec3(const vector3f_t vec)
{
	// TODO: They should be exactly the same, so we can probably just cast directly
	return (JPH_Vec3){
		.x = vec.x,
		.y = vec.y,
		.z = vec.z,
	};
}

static JPH_MotionType jph_motion_type(const physics_motion_type_t motion_type)
{
	// Should always match, but keep in function just in case
	return (JPH_MotionType) motion_type;
}

static JPH_Activation jph_activation(const bool activate)
{
	return (int) activate
		? JPH_Activation_Activate
		: JPH_Activation_DontActivate;
}

static void add_body(physics_engine_t *engine, const JPH_BodyCreationSettings *settings, const bool activate)
{
	const JPH_Activation activation = jph_activation(activate);
	const JPH_BodyID body = JPH_BodyInterface_CreateAndAddBody(engine->body_interface, settings, activation);
	engine->bodies[engine->num_bodies++] = body;
}

void physics_engine_add_box(physics_engine_t *engine, const box_config_t *config)
{
	const JPH_Vec3 jph_half_extents = jph_vec3(config->half_extents);
	JPH_BoxShape *shape = JPH_BoxShape_Create(&jph_half_extents, JPH_DEFAULT_CONVEX_RADIUS);

	const JPH_Vec3 jph_position = jph_vec3(config->position);
	JPH_BodyCreationSettings *settings = JPH_BodyCreationSettings_Create3(
		(JPH_Shape *) shape, &jph_position, nullptr,
		jph_motion_type(config->motion_type), config->layer
	);
	add_body(engine, settings, config->activate);
	JPH_BodyCreationSettings_Destroy(settings);
}

void physics_engine_add_sphere(physics_engine_t *engine, const sphere_config_t *config)
{
	JPH_SphereShape *shape = JPH_SphereShape_Create(config->radius);

	const JPH_Vec3 jph_position = jph_vec3(config->position);
	JPH_BodyCreationSettings *settings = JPH_BodyCreationSettings_Create3(
		(JPH_Shape *) shape, &jph_position, nullptr,
		jph_motion_type(config->motion_type), config->layer
	);
	add_body(engine, settings, config->activate);
	JPH_BodyCreationSettings_Destroy(settings);
}
