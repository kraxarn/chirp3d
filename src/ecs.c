#include "ecs.h"
#include "ecsosapi.h"
#include "logcategory.h"
#include "vector.h"

#include "flecs.h"

#include <SDL3/SDL_log.h>

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

ecs_world_t *ecs_create()
{
	log_debug_info();

	ecs_os_api_t os_api = ecs_os_api_create();
	ecs_os_set_api(&os_api);

	return ecs_init();
}

void ecs_destroy(ecs_world_t *world)
{
	ecs_fini(world);
}
