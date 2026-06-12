#include "ecs.h"
#include "assets.h"
#include "ecsosapi.h"
#include "logcategory.h"
#include "vector.h"

#include "flecs.h"

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

static void ChirpImport(ecs_world_t *mod_world)
{
	const ecs_entity_t scope = ecs_get_scope(mod_world);
	{
		ECS_MODULE(mod_world, Chirp);

		const ecs_entity_desc_t entity_desc = {
			.use_low_id = true,
			.name = "Assets",
			.symbol = "assets_t",
		};
		const ecs_component_desc_t component_desc = {
			.entity = ecs_entity_init(ecs_world(), &entity_desc),
			.type = (ecs_type_info_t){
				.size = ECS_SIZEOF(assets_t),
				.alignment = ECS_ALIGNOF(assets_t),
			},
		};
		ecs_component_init(mod_world, &component_desc);
	}
	ecs_set_scope(mod_world, scope);
}

void ecs_create()
{
	if (world != nullptr)
	{
		return;
	}

	log_debug_info();

	ecs_os_api_t os_api = ecs_os_api_create();
	ecs_os_set_api(&os_api);

	world = ecs_init();

	ECS_IMPORT(world, Chirp);

#ifdef FLECS_REST
	ecs_singleton_set(world, EcsRest, {0});
#endif

#ifdef FLECS_STATS
	ECS_IMPORT(world, FlecsStats);
#endif
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
