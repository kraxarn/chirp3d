#include "physics.h"
#include "logcategory.h"

#include "joltc.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>

typedef struct physics_engine_t
{
	JPH_JobSystem *job_system;
	JPH_PhysicsSystem *physics_system;
	JPH_BodyInterface *body_interface;
} physics_engine_t;

typedef enum object_layers_t: JPH_ObjectLayer
{
	OBJ_LAYER_NON_MOVING = 0,
	OBJ_LAYER_MOVING     = 1,
	OBJ_LAYER_COUNT      = 2,
} object_layers_t;

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

	engine->job_system = JPH_JobSystemThreadPool_Create(nullptr);

	JPH_ObjectLayerPairFilter *obj_filter = JPH_ObjectLayerPairFilterTable_Create(OBJ_LAYER_COUNT);
	JPH_ObjectLayerPairFilterTable_EnableCollision(obj_filter,
		OBJ_LAYER_NON_MOVING, OBJ_LAYER_MOVING
	);
	JPH_ObjectLayerPairFilterTable_EnableCollision(obj_filter,
		OBJ_LAYER_MOVING, OBJ_LAYER_NON_MOVING
	);

	// 1-to-1 mapping between object layers and broad-phase layers
	JPH_BroadPhaseLayerInterface *bp_interface = JPH_BroadPhaseLayerInterfaceTable_Create(
		OBJ_LAYER_COUNT, BP_LAYER_COUNT
	);
	JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer(bp_interface,
		OBJ_LAYER_NON_MOVING, BP_LAYER_NON_MOVING
	);
	JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer(bp_interface,
		OBJ_LAYER_MOVING, BP_LAYER_MOVING
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
