#include "physics.h"
#include "ecs.h"
#include "ecs/components.h"
#include "ecs/tags.h"

#include "flecs.h"

static void update_physics(ecs_iter_t *iter)
{
	const physics_engine_t *physics_engine = ecs_field(iter, physics_engine_t, 0);
	if (!physics_update(physics_engine, iter->delta_time))
	{
		ecs_set_error("Physics error", SDL_GetError());
	}
}

static void sync_physics(ecs_iter_t *iter)
{
	const physics_engine_t *physics_engine = ecs_field(iter, physics_engine_t, 0);
	const physics_body_id_t *body_ids = ecs_field(iter, physics_body_id_t, 2);
	projection_t *projections = ecs_field(iter, projection_t, 3);
	position_t *positions = ecs_field(iter, position_t, 4);
	rotation_t *rotations = ecs_field(iter, rotation_t, 5);

	for (Sint32 i = 0; i < iter->count; i++)
	{
		positions[i] = physics_body_position(physics_engine, body_ids[i]);

		const vector4f_t jph_rotation = physics_body_rotation(physics_engine, body_ids[i]);
		rotations[i] = (rotation_t){
			.x = jph_rotation.x,
			.y = jph_rotation.y,
			.z = jph_rotation.z,
		};

		projections[i].rebuild = true;
	}
}

void ecs_add_physics()
{
	ecs_system_init(ecs_world(), &(ecs_system_desc_t){
		.entity = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
			.name = "UpdatePhysics",
			.add = ecs_ids(ecs_dependson(ecs_phase(PHASE_PHYSICS_UPDATE))),
		}),
		.query.terms = {
			(ecs_term_t){.id = EcsPhysicsEngine, .inout = EcsIn},
		},
		.callback = update_physics,
	});

	ecs_system_init(ecs_world(), &(ecs_system_desc_t){
		.entity = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
			.name = "SyncPhysics",
			.add = ecs_ids(ecs_dependson(ecs_phase(PHASE_PHYSICS_SYNC))),
		}),
		.query.terms = {
			(ecs_term_t){.id = EcsPhysicsEngine, .src.id = EcsEngine, .inout = EcsIn},
			(ecs_term_t){.first.id = EcsChildOf, .second.name = "$instance"},
			(ecs_term_t){.id = EcsPhysicsBody, .src.name = "$instance", .inout = EcsIn},
			(ecs_term_t){.id = EcsProjection, .inout = EcsInOut},
			(ecs_term_t){.id = EcsPosition, .src.name = "$instance", .inout = EcsOut},
			(ecs_term_t){.id = EcsRotation, .src.name = "$instance", .inout = EcsOut},
		},
		.callback = sync_physics,
	});
}
