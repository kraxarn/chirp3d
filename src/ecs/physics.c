#include "physics.h"
#include "cast.h"
#include "ecs.h"
#include "ecs/components.h"

#include "flecs.h"
#include "box3d/box3d.h"
#include "box3d/id.h"
#include "flecs/addons/flecs_c.h"

/** \see physics_ctor */
static void init_physics([[maybe_unused]] ecs_iter_t *iter)
{
	ecs_add_id(ecs_world(), EcsPhysicsWorld, EcsPhysicsWorld);
}

static void update_physics(ecs_iter_t *iter)
{
	const b3WorldId world = *ecs_field(iter, b3WorldId, 0);

	// TODO: Test this with different frame-rates
	constexpr int sub_step_count = 4;
	b3World_Step(world, iter->delta_time, sub_step_count);
}

static void sync_physics(ecs_iter_t *iter)
{
	const b3BodyId *body_ids = ecs_field(iter, b3BodyId, 0);
	position_t *positions = ecs_field(iter, position_t, 1);
	projection_t *projections = ecs_field(iter, projection_t, 2);

	for (Sint32 i = 0; i < iter->count; i++)
	{
		const b3Pos b3_pos = b3Body_GetPosition(body_ids[i]);
		positions[i] = cast(vector3f_t, b3_pos);

		// TODO: Fix rotation

		projections[i].rebuild = true; // TODO: Do this in observer or something
	}
}

void ecs_add_physics()
{
	ecs_observer_init(ecs_world(), &(ecs_observer_desc_t){
		.query.terms = {
			(ecs_term_t){.id = EcsInit, .inout = EcsInOutFilter},
		},
		.events = {EcsOnSet},
		.callback = init_physics,
	});

	ecs_system_init(ecs_world(), &(ecs_system_desc_t){
		.entity = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
			.name = "UpdatePhysics",
			.add = ecs_ids(ecs_dependson(ecs_phase(PHASE_PHYSICS_UPDATE))),
		}),
		.query.terms = {
			(ecs_term_t){.id = EcsPhysicsWorld, .inout = EcsIn},
		},
		.callback = update_physics,
	});

	ecs_system_init(ecs_world(), &(ecs_system_desc_t){
		.entity = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
			.name = "SyncPhysics",
			.add = ecs_ids(ecs_dependson(ecs_phase(PHASE_PHYSICS_SYNC))),
		}),
		.query.terms = {
			(ecs_term_t){.id = EcsPhysicsBody, .inout = EcsIn},
			(ecs_term_t){.id = EcsPosition, .oper = EcsOptional, .inout = EcsInOut},
			(ecs_term_t){.id = EcsProjection, .src.name = "$node", .inout = EcsInOut},
			(ecs_term_t){.src.name = "$model", .first.id = EcsChildOf, .second.name = "$this"},
			(ecs_term_t){.src.name = "$node", .first.id = EcsChildOf, .second.name = "$model"},
		},
		.callback = sync_physics,
	});
}
