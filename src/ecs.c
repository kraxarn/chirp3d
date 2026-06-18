#include "ecs.h"
#include "assets.h"
#include "camera.h"
#include "ecsosapi.h"
#include "logcategory.h"
#include "physics.h"
#include "physicsconfig.h"
#include "windowconfig.h"

#include "flecs.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_cpuinfo.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_video.h>

static ecs_world_t *world = nullptr;
static ecs_entity_t phases[PHASE_COUNT];

static void log_debug_info()
{
	constexpr size_t temp_len = 160;
	char temp[temp_len] = {0};

#define append(str) do{if(temp[0]!='\0')SDL_strlcat(temp,", ",temp_len);SDL_strlcat(temp,str,temp_len);}while(false)

#ifdef FLECS_CPP
	append("cpp");
#endif

#ifdef FLECS_MODULE
	append("module");
#endif

#ifdef FLECS_SYSTEM
	append("system");
#endif

#ifdef FLECS_PIPELINE
	append("pipeline");
#endif

#ifdef FLECS_TIMER
	append("timer");
#endif

#ifdef FLECS_META
	append("meta");
#endif

#ifdef FLECS_UNITS
	append("units");
#endif

#ifdef FLECS_JSON
	append("json");
#endif

#ifdef FLECS_DOC
	append("doc");
#endif

#ifdef FLECS_HTTP
	append("http");
#endif

#ifdef FLECS_REST
	append("rest");
#endif

#ifdef FLECS_PARSER
	append("parser");
#endif

#ifdef FLECS_QUERY_DSL
	append("query_dsl");
#endif

#ifdef FLECS_SCRIPT
	append("script");
#endif

#ifdef FLECS_STATS
	append("stats");
#endif

#ifdef FLECS_METRICS
	append("metrics");
#endif

#ifdef FLECS_ALERTS
	append("alerts");
#endif

#ifdef FLECS_LOG
	append("log");
#endif

#ifdef FLECS_JOURNAL
	append("journal");
#endif

#ifdef FLECS_APP
	append("app");
#endif

#ifdef FLECS_OS_API_IMPL
	append("os_api_impl");
#endif

#undef append

	SDL_LogDebug(LOG_CATEGORY_ECS, "ECS addons: %s", temp);
}

static ecs_entity_t phase(const char *name)
{
	const ecs_entity_desc_t entity_desc = {
		.name = name,
		.add = ecs_ids(EcsPhase),
	};
	return ecs_entity_init(world, &entity_desc);
}

static void phase_depend(const phase_t source, const phase_t target)
{
	ecs_add_pair(world, phases[source], EcsDependsOn, phases[target]);
}

static void create_pipeline()
{
	const ecs_pipeline_desc_t pipeline_desc = {
		.query.terms = {
			(ecs_term_t){
				.id = EcsSystem,
			},
			(ecs_term_t){
				.id = EcsPhase,
				.src.id = EcsCascade,
				.trav = EcsDependsOn,
			},
		},
	};
	const ecs_entity_t pipeline = ecs_pipeline_init(world, &pipeline_desc);
	ecs_set_pipeline(world, pipeline);

	phases[PHASE_UPDATE] = phase("Update");
	phases[PHASE_PHYSICS_UPDATE] = phase("PhysicsUpdate");
	phases[PHASE_PHYSICS_SYNC] = phase("PhysicsSync");
	phases[PHASE_RENDER_BEGIN] = phase("RenderBegin");
	phases[PHASE_RENDER] = phase("Render");
	phases[PHASE_RENDER_END] = phase("RenderEnd");

	phase_depend(PHASE_PHYSICS_UPDATE, PHASE_UPDATE);
	phase_depend(PHASE_PHYSICS_SYNC, PHASE_PHYSICS_UPDATE);
	phase_depend(PHASE_RENDER_BEGIN, PHASE_PHYSICS_SYNC);
	phase_depend(PHASE_RENDER, PHASE_RENDER_BEGIN);
	phase_depend(PHASE_RENDER_END, PHASE_RENDER);
}

#define component(n,s)									\
	do {												\
		const ecs_entity_desc_t e_desc = {				\
			.use_low_id = true,							\
			.name = n,									\
			.symbol = #s,								\
		};												\
		const ecs_component_desc_t c_desc = {			\
			.entity = ecs_entity_init(world, &e_desc),	\
			.type = (ecs_type_info_t){					\
				.size = ECS_SIZEOF(s),					\
				.alignment = ECS_ALIGNOF(s),			\
			},											\
		};												\
		ecs_component_init(world, &c_desc);				\
	} while (false)

#define tag(n)									\
	do {										\
		const ecs_entity_desc_t entity_desc = {	\
			.name = n,							\
		};										\
		ecs_entity_init(world, &entity_desc);	\
	} while (false)

static void module([[maybe_unused]] ecs_world_t *ewt)
{
	const ecs_component_desc_t desc = {};
	const ecs_entity_t mod = ecs_module_init(world, "Chirp", &desc);
	const ecs_entity_t scope = ecs_set_scope(world, mod);
	{
		component("Assets", assets_t);
		component("Init", init_flags_t);
		component("WindowConfig", window_config_t);
		component("Window", window_t*);
		component("GpuDevice", gpu_device_t*);
		component("GpuGraphicsPipeline", gpu_graphics_pipeline_t*);
		component("DepthTexture", depth_texture_t*);
		component("Camera", camera_t);
		component("PhysicsConfig", physics_config_t);
		component("PhysicsEngine", physics_engine_t);
		component("Model", model_t);
		component("InstanceOf", instance_of_index_t);
		component("PhysicsBody", physics_body_id_t);

		component("Rotation", rotation_t);
		component("Position", position_t);
		component("Scale", scale_t);
		component("Projection", projection_t);

		tag("Scene");

		create_pipeline();
	}
	ecs_set_scope(world, scope);
}

static void on_init_set([[maybe_unused]] ecs_iter_t *iter)
{
	ecs_os_api_t os_api = ecs_os_api_create();
	ecs_os_set_api(&os_api);

	const int cores = SDL_GetNumLogicalCPUCores();
	ecs_set_threads(world, cores);
	SDL_LogDebug(LOG_CATEGORY_ECS, "Using %d ECS threads", cores);

#ifdef FLECS_REST
	ecs_singleton_set(world, EcsRest, {0});
#endif

#ifdef FLECS_STATS
	ECS_IMPORT(world, FlecsStats);
#endif
}

void ecs_create()
{
	if (world != nullptr)
	{
		return;
	}

	log_debug_info();

	world = ecs_init();
	ecs_import(world, module, "chirp");

	// SDL has to initialise before we set up OS-specific stuff
	ecs_observer_init(world, &(ecs_observer_desc_t){
		.query.terms = {
			(ecs_term_t){
				.id = ecs_lookup(world, "chirp.Init"),
			}
		},
		.events = {EcsOnSet},
		.callback = on_init_set,
	});
}

void ecs_destroy()
{
	ecs_fini(world);
	world = nullptr;
}

ecs_world_t *ecs_world()
{
	return world;
}

ecs_entity_t ecs_phase(const phase_t phase)
{
	const ecs_entity_t entity = phases[phase];
	SDL_assert(entity != 0);
	return entity;
}

const void *ecs_const_data(const char *name)
{
	const ecs_entity_t entity = ecs_lookup(world, name);
	if (entity == 0)
	{
		return nullptr;
	}

	return ecs_get_id(world, entity, entity);
}

void *ecs_mut_data_ptr(const char *name)
{
	const void *data = ecs_const_data(name);
	if (data == nullptr)
	{
		return nullptr;
	}

	return *((void**) data);
}

void *ecs_mut_data(const char *name)
{
	const ecs_entity_t entity = ecs_lookup(world, name);
	if (entity == 0)
	{
		return nullptr;
	}

	return ecs_get_mut_id(world, entity, entity);
}
