#include "assets.h"
#include "ecs.h"

#include "flecs.h"

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_filesystem.h>

static void on_file_opened(void *userdata,
	const char *const *filelist, [[maybe_unused]] int filter)
{
	if (filelist == nullptr || filelist[0] == nullptr)
	{
		return;
	}

	assets_t assets;
	if (assets_create(filelist[0], userdata, &assets))
	{
		const ecs_entity_t engine = ecs_lookup(ecs_world(), "chirp.Engine");
		const ecs_id_t assets_id = ecs_lookup(ecs_world(), "chirp.Assets");
		ecs_set_id(ecs_world(), engine, assets_id,
			sizeof(assets_t), &assets);
	}
}

static void create_assets(ecs_iter_t *iter)
{
	const input_t *input = ecs_field(iter, input_t, 0);

	const char *base_path = SDL_GetBasePath();
	const size_t path_len = SDL_strlen(base_path) + SDL_arraysize("assets.nest") + 1;
	char *path = SDL_calloc(path_len, sizeof(char));
	SDL_strlcat(path, base_path, path_len);
	SDL_strlcat(path, "assets.nest", path_len);

	assets_t assets;
	if (assets_create(path, input, &assets))
	{
		SDL_free(path);

		const ecs_entity_t engine = ecs_lookup(ecs_world(), "chirp.Engine");
		const ecs_id_t assets_id = ecs_lookup(ecs_world(), "chirp.Assets");
		ecs_set_id(ecs_world(), engine, assets_id,
			sizeof(assets_t), &assets);

		return;
	}
	SDL_free(path);

	const SDL_DialogFileFilter filters[] = {
		(SDL_DialogFileFilter){
			.name = "Packed assets",
			.pattern = "nest",
		},
	};

	SDL_ShowOpenFileDialog(on_file_opened, (void*) input, nullptr,
		filters, SDL_arraysize(filters), nullptr, false);
}

void ecs_add_assets()
{
	const ecs_observer_desc_t observer_desc = {
		.query.terms = {
			(ecs_term_t){
				.id = ecs_lookup(ecs_world(), "chirp.Input"),
				.inout = EcsIn,
			},
		},
		.events = {EcsOnSet},
		.callback = create_assets,
	};
	ecs_observer_init(ecs_world(), &observer_desc);
}
