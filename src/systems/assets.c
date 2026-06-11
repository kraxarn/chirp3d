#include "assets.h"
#include "ecs.h"
#include "systems.h"

#include "flecs.h"

#include <SDL3/SDL_dialog.h>
#include <SDL3/SDL_filesystem.h>

static void on_file_opened([[maybe_unused]] void *userdata,
	const char *const *filelist, [[maybe_unused]] int filter)
{
	if (filelist == nullptr || filelist[0] == nullptr)
	{
		return;
	}

	assets_t assets;
	if (assets_create(filelist[0], &assets))
	{
		const ecs_entity_t entity = ecs_lookup(ecs_world(), "Assets");
		ecs_set_id(ecs_world(), entity, entity,
			sizeof(Assets), &assets);
	}
}

void system_register_assets()
{
	const ecs_entity_desc_t entity_desc = {
		.use_low_id = true,
		.name = "Assets",
		.symbol = "Assets",
	};
	const ecs_component_desc_t component_desc = {
		.entity = ecs_entity_init(ecs_world(), &entity_desc),
		.type = (ecs_type_info_t){
			.size = ECS_SIZEOF(Assets),
			.alignment = ECS_ALIGNOF(Assets),
		},
	};
	ecs_component_init(ecs_world(), &component_desc);

	const char *base_path = SDL_GetBasePath();
	const size_t path_len = SDL_strlen(base_path) + SDL_arraysize("assets.nest") + 1;
	char *path = SDL_calloc(path_len, sizeof(char));
	SDL_strlcat(path, base_path, path_len);
	SDL_strlcat(path, "assets.nest", path_len);

	assets_t assets;
	if (assets_create(path, &assets))
	{
		SDL_free(path);

		const ecs_entity_t entity = ecs_lookup(ecs_world(), "Assets");
		ecs_set_id(ecs_world(), entity, entity,
			sizeof(Assets), &assets);

		return;
	}
	SDL_free(path);

	const SDL_DialogFileFilter filters[] = {
		(SDL_DialogFileFilter){
			.name = "Packed assets",
			.pattern = "nest",
		},
	};

	SDL_ShowOpenFileDialog(on_file_opened, nullptr, nullptr,
		filters, SDL_arraysize(filters), nullptr, false);
}
