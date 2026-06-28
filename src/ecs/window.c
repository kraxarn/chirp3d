#include "assets.h"
#include "ecs.h"
#include "ecs/components.h"
#include "ecs/tags.h"

#include "flecs.h"

static void load_window_config(ecs_iter_t *iter)
{
	const assets_t *assets = ecs_field(iter, assets_t, 0);

	const window_config_t window_config = assets_window_config(assets);
	ecs_set_id(ecs_world(), EcsEngine, EcsWindowConfig,
		sizeof(window_config_t), &window_config);
}

static void create_window(ecs_iter_t *iter)
{
	const window_config_t *config = ecs_field(iter, window_config_t, 0);

	SDL_Window *window = SDL_CreateWindow(config->title,
		config->size.x, config->size.y,
		window_config_flags(*config)
	);

	if (window == nullptr)
	{
		ecs_set_error("Window error", SDL_GetError());
		return;
	}

	ecs_set_id(ecs_world(), EcsEngine, EcsWindow,
		sizeof(SDL_Window*), (const void*) &window);
}

void ecs_add_window()
{
	const ecs_observer_desc_t observer_desc[] = {
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){.id = EcsAssets},
			},
			.events = {EcsOnSet},
			.callback = load_window_config,
		},
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){.id = EcsWindowConfig},
			},
			.events = {EcsOnSet},
			.callback = create_window,
		},
	};

	ecs_observer_init_all(observer_desc);
}
