#include "scriptengine.h"
#include "assets.h"
#include "ecs.h"
#include "ecs/components.h"
#include "ecs/tags.h"

#include "flecs.h"

static void create_script_engine([[maybe_unused]] ecs_iter_t *iter)
{
	const py_vm_index_t vm_index = script_engine_create();

	ecs_set_id(ecs_world(), EcsEngine, EcsScriptEngine,
		sizeof(py_vm_index_t), &vm_index);
}

static void load_main_script(ecs_iter_t *iter)
{
	const assets_t *assets = ecs_field(iter, assets_t, 0);

	SDL_IOStream *script_stream = assets_load_script(assets, "main");
	if (script_stream == nullptr)
	{
		ecs_set_error("Script error", SDL_GetError());
		return;
	}

	if (!script_engine_exec("main", script_stream, true))
	{
		ecs_set_error("Script error", SDL_GetError());
	}
}

void ecs_add_script_engine()
{
	const ecs_observer_desc_t observer_desc[] = {
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){.id = EcsInit, .inout = EcsInOutNone},
			},
			.events = {EcsOnSet},
			.callback = create_script_engine,
		},
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){.id = EcsAssets, .inout = EcsIn},
				(ecs_term_t){.id = EcsScriptEngine, .inout = EcsInOutNone},
			},
			.events = {EcsOnSet},
			.callback = load_main_script,
		},
	};

	ecs_observer_init_all(observer_desc);
}
