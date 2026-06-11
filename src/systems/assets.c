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
		ECS_COMPONENT(ecs_world(), Assets);
		ecs_singleton_set(ecs_world(), Assets, {
			.stream = assets.stream,
			.window_config = assets.window_config,
			.desc = assets.desc,
			}
		);
	}
}

void system_register_assets()
{
	ECS_COMPONENT(ecs_world(), Assets);

	const char *base_path = SDL_GetBasePath();
	const size_t path_len = SDL_strlen(base_path) + SDL_arraysize("assets.nest") + 1;
	char *path = SDL_calloc(path_len, sizeof(char));
	SDL_strlcat(path, base_path, path_len);
	SDL_strlcat(path, "assets.nest", path_len);

	assets_t assets;
	if (assets_create(path, &assets))
	{
		SDL_free(path);
		ecs_singleton_set(ecs_world(), Assets, {
			.stream = assets.stream,
			.window_config = assets.window_config,
			.desc = assets.desc,
			}
		);
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
