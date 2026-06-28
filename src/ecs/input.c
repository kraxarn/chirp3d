#include "input.h"
#include "ecs.h"
#include "ecs/components.h"
#include "ecs/tags.h"

#include "flecs.h"

static void create_input([[maybe_unused]] ecs_iter_t *iter)
{
	input_t input;
	if (!input_create(&input))
	{
		ecs_set_error("Input error", SDL_GetError());
		return;
	}

	ecs_set_id(ecs_world(), EcsEngine, EcsInput,
		sizeof(input_t), &input);
}

void ecs_add_input()
{
	const ecs_observer_desc_t observer_desc = {
		.query.terms = {
			(ecs_term_t){.id = EcsInit, .inout = EcsInOutNone},
		},
		.events = {EcsOnSet},
		.callback = create_input,
	};
	ecs_observer_init(ecs_world(), &observer_desc);
}
