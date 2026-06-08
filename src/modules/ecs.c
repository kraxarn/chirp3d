#include "logcategory.h"
#include "modules.h"

#include "flecs.h"
#include "pocketpy.h"

#include <SDL3/SDL_log.h>

typedef enum
{
	PHASE_INVALID     = 0,
	PHASE_ON_LOAD     = 1,
	PHASE_POST_LOAD   = 2,
	PHASE_PRE_UPDATE  = 3,
	PHASE_ON_UPDATE   = 4,
	PHASE_ON_VALIDATE = 5,
	PHASE_POST_UPDATE = 6,
	PHASE_PRE_STORE   = 7,
	PHASE_ON_STORE    = 8,
} phase_t;

typedef py_TValue *py_iter_func_t;

static ecs_world_t *world = nullptr;

static ecs_entity_t phase_entity(const phase_t phase)
{
	switch (phase)
	{
		case PHASE_ON_LOAD:
			return EcsOnLoad;

		case PHASE_POST_LOAD:
			return EcsPostLoad;

		case PHASE_PRE_UPDATE:
			return EcsPreUpdate;

		case PHASE_ON_UPDATE:
			return EcsOnUpdate;

		case PHASE_ON_VALIDATE:
			return EcsOnValidate;

		case PHASE_POST_UPDATE:
			return EcsPostUpdate;

		case PHASE_PRE_STORE:
			return EcsPreStore;

		case PHASE_ON_STORE:
			return EcsOnStore;

		default:
			return 0;
	}
}

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
			.size = ECS_SIZEOF(void*),
			.alignment = ECS_ALIGNOF(void*),
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
	// TODO
}

static bool dec_system(const int argc, py_TValue *argv)
{
	static phase_t phase = PHASE_INVALID;
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

	if (argv->type == tp_function
		&& phase != PHASE_INVALID
		&& query != nullptr)
	{
		PY_CHECK_ARGC(1);

		const ecs_entity_desc_t entity_desc = {
			.id = 0,
			.name = nullptr,
		};
		const ecs_system_desc_t system_desc = {
			.entity = ecs_entity_init(world, &entity_desc),
			.query = (ecs_query_desc_t){
				.expr = "Position, [in] Velocity",
			},
			.phase = phase_entity(phase),
			.callback = system_run,
		};
		const ecs_entity_t entity = ecs_system_init(world, &system_desc);
		if (entity == 0)
		{
			return RuntimeError("Failed to create system");
		}

		ECS_COMPONENT(world, py_iter_func_t);
		ecs_set_id(world, entity, ecs_id(py_iter_func_t), sizeof(py_iter_func_t*), argv);

		SDL_LogDebug(LOG_CATEGORY_SCRIPT, "Added system (%s): %s",
			ecs_get_name(world, phase_entity(phase)),
			query[0] == '\0' ? "<empty>" : query
		);

		phase = PHASE_INVALID;
		query = nullptr;

		py_newnone(py_retval());
		return true;
	}

	return TypeError("expected 'int', got '%t'", argv->type);
}

// Not necessary (at all), but it cleans up a lot of single-line functions
#define phase_getter(name, phase)										\
	static bool phase_getter_##name(const int argc, py_TValue *argv) {	\
		PY_CHECK_ARGC(1);												\
		PY_CHECK_ARG_TYPE(0, tp_property);								\
		py_newint(py_retval(), phase);									\
		return true;													\
	}

phase_getter(on_load, PHASE_ON_LOAD)
phase_getter(post_load, PHASE_POST_LOAD)
phase_getter(pre_update, PHASE_PRE_UPDATE)
phase_getter(on_update, PHASE_ON_UPDATE)
phase_getter(on_validate, PHASE_ON_VALIDATE)
phase_getter(post_update, PHASE_POST_UPDATE)
phase_getter(pre_store, PHASE_PRE_STORE)
phase_getter(on_store, PHASE_ON_STORE)

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

	add_phase(mod);
}
