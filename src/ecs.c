#include "ecs.h"
#include "assets.h"
#include "ecsosapi.h"
#include "logcategory.h"

#include "flecs.h"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_log.h>

static ecs_world_t *world = nullptr;

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

static void module([[maybe_unused]] ecs_world_t *ewt)
{
	const ecs_component_desc_t desc = {};
	const ecs_entity_t mod = ecs_module_init(world, "Chirp", &desc);
	const ecs_entity_t scope = ecs_set_scope(world, mod);
	{
		component("Assets", assets_t);
		component("Init", SDL_InitFlags);
	}
	ecs_set_scope(world, scope);
}

static void on_init_set([[maybe_unused]] ecs_iter_t *iter)
{
	ecs_os_api_t os_api = ecs_os_api_create();
	ecs_os_set_api(&os_api);

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
