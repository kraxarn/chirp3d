#include "assets.h"
#include "ecs.h"
#include "logcategory.h"
#include "ecs/components.h"
#include "ecs/tags.h"

#include "flecs.h"

#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_log.h>

#include "ecs/entities.h"

[[nodiscard]]
static ecs_entity_t models_entity()
{
	const ecs_entity_desc_t entity_desc = {
		.name = "Model",
	};
	return ecs_entity_init(ecs_world(), &entity_desc);
}

static Uint16 fix_entity_name(const char *name)
{
	Uint16 changes = 0;

	char *str;
	while ((str = SDL_strchr(name, '.')) != nullptr)
	{
		*str = '_';
		changes++;
	}

	return changes;
}

static ecs_entity_t load_model(const char *name)
{
	SDL_LogInfo(LOG_CATEGORY_ECS, "Loading model: '%s'", name); // TODO: Debug

	const assets_t *assets = ecs_get_id(ecs_world(), EcsEngine, EcsAssets);
	SDL_GPUDevice *gpu_device = *((SDL_GPUDevice**) ecs_get_mut_id(ecs_world(), EcsEngine, EcsGpuDevice));

	model_t model;
	if (!assets_load_model(assets, gpu_device, name, &model))
	{
		SDL_LogError(LOG_CATEGORY_MODEL, "Failed to load model '%s': %s",
			name, SDL_GetError());
		return 0;
	}

	const ecs_entity_t entity = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
		.name = name,
	});
	ecs_add_pair(ecs_world(), entity, EcsChildOf, models_entity());

	ecs_set_id(ecs_world(), entity, EcsModel,
		sizeof(model_t), &model);

	for (size_t i = 0; i < model.node_count; i++)
	{
		char *node_name = SDL_strdup(model_node_name(&model, i));
		if (fix_entity_name(node_name) > 0)
		{
			SDL_LogWarn(LOG_CATEGORY_MODEL, "Renamed invalid entity name '%s' to '%s' in '%s'",
				model_node_name(&model, i), node_name, name);
		}
		const ecs_entity_t node = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
			.name = node_name,
		});
		SDL_free(node_name);

		ecs_add_pair(ecs_world(), node, EcsChildOf, entity);

		const position_t position = model_node_translation(&model, i);
		ecs_set_id(ecs_world(), node, EcsPosition,
			sizeof(position_t), &position);

		const world_transform_t world_transform = model_node_world_transform(&model, i);
		ecs_set_id(ecs_world(), node, EcsWorldTransform,
			sizeof(world_transform_t), &world_transform);
	}

	return entity;
}

static void create_instance(const ecs_entity_t entity, const ecs_entity_t model)
{
	SDL_LogInfo(LOG_CATEGORY_ECS, "Creating new instance of model '%s'", // TODO: Debug
		ecs_get_name(ecs_world(), model));

	const ecs_entity_t instance = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
		// Should be fine to copy name, they probably have different paths
		.name = ecs_get_name(ecs_world(), model),
		.parent = entity,
		.add = (ecs_id_t[]){
			ecs_pair(EcsInstanceOf, model),
			ecs_ids_end,
		},
	});

	ecs_iter_t iter = ecs_children(ecs_world(), model);
	while (ecs_children_next(&iter))
	{
		for (Sint32 i = 0; i < iter.count; i++)
		{
			const ecs_entity_t child = iter.entities[i];

			const ecs_entity_t node = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
				.name = ecs_get_name(ecs_world(), child),
				.parent = instance,
			});

			const projection_t projection = {.rebuild = true};
			ecs_set_id(ecs_world(), node, EcsProjection,
				sizeof(projection_t), &projection);

			ecs_add_pair(ecs_world(), node, EcsInstanceOf, child);
		}
	}
	ecs_iter_fini(&iter);
}

static void instance_model(ecs_iter_t *iter)
{
	// Only do one at a time for now, just to avoid
	// handling for loading the same model multiple times
	// (and to balance load or some other excuse)
	// TODO: If we return after loading model, we can load all instances at once

	SDL_assert(iter->count >= 1);

	char *name = ecs_field(iter, model_instance_t, 0)->name;

	const ecs_entity_t entity = iter->entities[0];
	SDL_assert(entity != 0);

	const ecs_entity_t model = ecs_lookup_child(ecs_world(), models_entity(), name);
	if (model == 0)
	{
		if (load_model(name) == 0)
		{
			// Don't try to load indefinitely
			ecs_remove_id(ecs_world(), entity, EcsModelInstance);
		}
		return; // Deferred, I don't really like this, but it works
	}

	create_instance(entity, model);
	SDL_free(name); // TODO: Maybe do this somewhere else
	ecs_remove_id(ecs_world(), entity, EcsModelInstance);
}

void ecs_add_models()
{
	ecs_system_init(ecs_world(), &(ecs_system_desc_t){
		.entity = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
			.name = "LoadModels",
			.add = ecs_ids(ecs_dependson(ecs_phase(PHASE_RENDER_BEGIN))),
		}),
		.query.terms = {
			(ecs_term_t){.id = EcsModelInstance, .inout = EcsInOut},
		},
		.callback = instance_model,
	});
}
