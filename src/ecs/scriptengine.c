#include "scriptengine.h"
#include "assets.h"
#include "ecs.h"

#include "flecs.h"

static void create_script_engine([[maybe_unused]] ecs_iter_t *iter)
{
	const py_vm_index_t vm_index = script_engine_create();

	const ecs_entity_t engine = ecs_lookup(ecs_world(), "chirp.Engine");
	const ecs_id_t script_engine_id = ecs_lookup(ecs_world(), "chirp.ScriptEngine");

	ecs_set_id(ecs_world(), engine, script_engine_id,
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
				(ecs_term_t){
					.id = ecs_lookup(ecs_world(), "chirp.Init"),
					.inout = EcsInOutNone,
				},
			},
			.events = {EcsOnSet},
			.callback = create_script_engine,
		},
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){
					.id = ecs_lookup(ecs_world(), "chirp.Assets"),
					.inout = EcsIn,
				},
				(ecs_term_t){
					.id = ecs_lookup(ecs_world(), "chirp.ScriptEngine"),
					.inout = EcsInOutNone,
				},
			},
			.events = {EcsOnSet},
			.callback = load_main_script,
		},
	};

	ecs_observer_init_all(observer_desc);
}
