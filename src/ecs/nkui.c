#include "nkui.h"
#include "ecs.h"
#include "logcategory.h"
#include "ecs/components.h"

#include "flecs.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_video.h>

static void init(ecs_iter_t *iter)
{
	SDL_Window *window = *ecs_field(iter, SDL_Window*, 0);
	SDL_GPUDevice *device = *ecs_field(iter, SDL_GPUDevice*, 0);

	nk_context_t context;
	if (!nkui_init(window, device, &context))
	{
		SDL_LogError(LOG_CATEGORY_UI, "Failed to load UI: %s",
			SDL_GetError());
		return;
	}

	ecs_set_id(ecs_world(), EcsNkContext, EcsNkContext,
		sizeof(nk_context_t), &context);
}

void ecs_add_nkui()
{
	ecs_observer_init(ecs_world(), &(ecs_observer_desc_t){
		.query.terms = {
			(ecs_term_t){.id = EcsWindow},
			(ecs_term_t){.id = EcsGpuDevice},
		},
		.events = {EcsOnSet},
		.callback = init,
	});
}
