#include "logcategory.h"
#include "modules.h"

#include "flecs.h"
#include "pocketpy.h"

#include <SDL3/SDL_log.h>

typedef py_TValue py_iter_func_t;

static ecs_world_t *world = nullptr;

static bool dec_component(const int argc, py_TValue *argv)
{
	PY_CHECK_ARGC(1);
	PY_CHECK_ARG_TYPE(0, tp_type);

	const char *name = py_tpname(py_totype(argv));

	const ecs_entity_desc_t entity_desc = {
		.id = 0,
		.use_low_id = true,
		.name = name,
		.symbol = name,
	};
	const ecs_component_desc_t component_desc = {
		.entity = ecs_entity_init(world, &entity_desc),
		.type = (ecs_type_info_t){
			.size = ECS_SIZEOF(py_TValue),
			.alignment = ECS_ALIGNOF(py_TValue),
		},
	};
	const ecs_entity_t entity = ecs_component_init(world, &component_desc);
	if (entity == 0)
	{
		return RuntimeError("Failed to create component %s", name);
	}

	SDL_LogDebug(LOG_CATEGORY_SCRIPT, "Added entity: %s (ID %lu)", name, entity);

	py_assign(py_retval(), py_arg(0));
	return true;
}

static void system_run(ecs_iter_t *iter)
{
	const ecs_entity_t system = iter->system;
	ECS_COMPONENT(world, py_iter_func_t);
	const auto iter_func = ecs_get(world, system, py_iter_func_t);
	py_call((py_Ref) iter_func, 0, nullptr);
}

static bool dec_system(const int argc, py_TValue *argv)
{
	static ecs_entity_t phase = 0;
	static const char *query = nullptr;

	if (argv->type == tp_property)
	{
		PY_CHECK_ARGC(2);

		if (!py_call(py_getslot(argv, 0), 1, argv))
		{
			return false;
		}

		phase = py_toint(py_retval());
		query = py_tostr(py_arg(1));
		py_newnativefunc(py_retval(), dec_system);
		return true;
	}

	if (argv->type == tp_function && phase > 0 && query != nullptr)
	{
		PY_CHECK_ARGC(1);

		const ecs_entity_desc_t entity_desc = {
			.id = 0,
			.name = nullptr,
		};
		const ecs_system_desc_t system_desc = {
			.entity = ecs_entity_init(world, &entity_desc),
			.query = (ecs_query_desc_t){
				.expr = query,
			},
			.phase = phase,
			.callback = system_run,
		};
		const ecs_entity_t entity = ecs_system_init(world, &system_desc);
		if (entity == 0)
		{
			return RuntimeError("Failed to create system");
		}

		ECS_COMPONENT(world, py_iter_func_t);
		ecs_set_id(world, entity, ecs_id(py_iter_func_t), sizeof(py_iter_func_t), argv);

		SDL_LogDebug(LOG_CATEGORY_SCRIPT, "Added system %s: \"%s\" (ID %lu)",
			ecs_get_name(world, phase), query, entity);

		phase = 0;
		query = nullptr;

		py_newnone(py_retval());
		return true;
	}

	return TypeError("expected 'int', got '%t'", argv->type);
}

static bool spawn(const int argc, py_TValue *argv)
{
	if (argc == 1)
	{
		PY_CHECK_ARG_TYPE(0, tp_tuple);
	}
	else if (argc > 1)
	{
		PY_CHECK_ARGC(1);
	}

	const ecs_entity_t entity = ecs_new(world);

	if (argc > 0)
	{
		for (int i = 0; i < py_tuple_len(argv); i++)
		{
			const py_TValue *item = py_tuple_getitem(argv, i);
			const ecs_entity_t component = ecs_lookup(world, py_tpname(item->type));
			if (component == 0)
			{
				return TypeError("unexpected type '%t'", item->type);
			}
			ecs_set_id(world, entity, component, sizeof(py_TValue), item);
		}
	}

	py_newint(py_retval(), (py_i64) entity);
	return true;
}

// Not necessary (at all), but it cleans up a lot of single-line functions
#define phase_getter(name, phase)										\
	static bool phase_getter_##name(const int argc, py_TValue *argv) {	\
		PY_CHECK_ARGC(1);												\
		PY_CHECK_ARG_TYPE(0, tp_property);								\
		py_newint(py_retval(), phase);									\
		return true;													\
	}

phase_getter(on_load, EcsOnLoad)
phase_getter(post_load, EcsPostLoad)
phase_getter(pre_update, EcsPreUpdate)
phase_getter(on_update, EcsOnUpdate)
phase_getter(on_validate, EcsOnValidate)
phase_getter(post_update, EcsPostUpdate)
phase_getter(pre_store, EcsPreStore)
phase_getter(on_store, EcsOnStore)

static void add_phase(py_TValue *mod)
{
	const py_Type type = py_newtype("Phase", tp_object, mod, nullptr);

	py_bindproperty(type, "ON_LOAD", phase_getter_on_load, nullptr);
	py_bindproperty(type, "POST_LOAD", phase_getter_post_load, nullptr);
	py_bindproperty(type, "PRE_UPDATE", phase_getter_pre_update, nullptr);
	py_bindproperty(type, "ON_UPDATE", phase_getter_on_update, nullptr);
	py_bindproperty(type, "ON_VALIDATE", phase_getter_on_validate, nullptr);
	py_bindproperty(type, "POST_UPDATE", phase_getter_post_update, nullptr);
	py_bindproperty(type, "PRE_STORE", phase_getter_pre_store, nullptr);
	py_bindproperty(type, "ON_STORE", phase_getter_on_store, nullptr);
}

void add_module_ecs(ecs_world_t *ecs_world)
{
	world = ecs_world;

	py_TValue *mod = py_newmodule("ecs");

	py_bindfunc(mod, "component", dec_component);
	py_bind(mod, "system(q, s='')", dec_system);
	py_bind(mod, "spawn(*args)", spawn);

	add_phase(mod);
}
