#include "assets.h"
#include "ecs.h"
#include "systems.h"
#include "logcategory.h"

#include "flecs.h"

#include <SDL3/SDL_log.h>

static void on_assets_set(ecs_iter_t *iter)
{
	const assets_t *assets = ecs_field(iter, assets_t, 0);

	const ecs_entity_t engine = ecs_lookup(ecs_world(), "chirp.Engine");
	const ecs_id_t window_config_id = ecs_lookup(ecs_world(), "chirp.WindowConfig");

	const window_config_t window_config = assets_window_config(assets);
	ecs_set_id(ecs_world(), engine, window_config_id,
		sizeof(window_config_t), &window_config);
}

static void on_window_config_set(ecs_iter_t *iter)
{
	const window_config_t *config = ecs_field(iter, window_config_t, 0);

	SDL_Window *window = SDL_CreateWindow(config->title,
		config->size.x, config->size.y,
		window_config_flags(*config)
	);

	if (window == nullptr)
	{
		SDL_LogError(LOG_CATEGORY_CORE, "Failed to create window: %s", SDL_GetError());
		return;
	}

	const ecs_entity_t engine = ecs_lookup(ecs_world(), "chirp.Engine");
	const ecs_id_t window_id = ecs_lookup(ecs_world(), "chirp.Window");

	ecs_set_id(ecs_world(), engine, window_id,
		sizeof(SDL_Window*), (void*) &window);
}

void system_register_window()
{
	const ecs_observer_desc_t observer_desc[] = {
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){
					.id = ecs_lookup(ecs_world(), "chirp.Assets"),
				},
			},
			.events = {EcsOnSet},
			.callback = on_assets_set,
		},
		(ecs_observer_desc_t){
			.query.terms = {
				(ecs_term_t){
					.id = ecs_lookup(ecs_world(), "chirp.WindowConfig"),
				},
			},
			.events = {EcsOnSet},
			.callback = on_window_config_set,
		},
	};

	ecs_observer_init(ecs_world(), observer_desc + 0);
	ecs_observer_init(ecs_world(), observer_desc + 1);
}
