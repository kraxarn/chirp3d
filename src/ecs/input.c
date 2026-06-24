#include "input.h"
#include "ecs.h"

#include "flecs.h"

static void create_input([[maybe_unused]] ecs_iter_t *iter)
{
	input_t input;
	if (!input_create(&input))
	{
		ecs_set_error("Input error", SDL_GetError());
		return;
	}

	const ecs_entity_t engine = ecs_lookup(ecs_world(), "chirp.Engine");
	const ecs_id_t input_id = ecs_lookup(ecs_world(), "chirp.Input");
	ecs_set_id(ecs_world(), engine, input_id,
		sizeof(input_t), &input);
}

void ecs_add_input()
{
	const ecs_observer_desc_t observer_desc = {
		.query.terms = {
			(ecs_term_t){
				.id = ecs_lookup(ecs_world(), "chirp.Init"),
				.inout = EcsInOutNone,
			},
		},
		.events = {EcsOnSet},
		.callback = create_input,
	};
	ecs_observer_init(ecs_world(), &observer_desc);
}
