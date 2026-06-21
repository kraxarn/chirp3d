#include "physics.h"
#include "ecs.h"
#include "systems.h"
#include "logcategory.h"

#include "flecs.h"

#include <SDL3/SDL_log.h>

static void update_physics(ecs_iter_t *iter)
{
	const physics_engine_t *physics_engine = ecs_field(iter, physics_engine_t, 0);
	if (!physics_update(physics_engine, iter->delta_time))
	{
		SDL_LogError(LOG_CATEGORY_PHYSICS, "Failed to update physics: %s", SDL_GetError());
	}
}

void system_register_physics()
{
	const ecs_id_t physics_engine_id = ecs_lookup(ecs_world(), "chirp.PhysicsEngine");

	ecs_system_init(ecs_world(), &(ecs_system_desc_t){
		.entity = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
			.add = ecs_ids(ecs_dependson(ecs_phase(PHASE_PHYSICS_UPDATE))),
		}),
		.query.terms = {
			{.id = physics_engine_id},
		},
		.callback = update_physics,
	});
}
