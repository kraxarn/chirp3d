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
	SDL_GPUDevice *device = *ecs_field(iter, SDL_GPUDevice*, 1);

	nkui_context_t context;
	if (!nkui_init(window, device, &context))
	{
		SDL_LogError(LOG_CATEGORY_UI, "Failed to load UI: %s",
			SDL_GetError());
		return;
	}

	ecs_set_id(ecs_world(), EcsNkContext, EcsNkContext,
		sizeof(nkui_context_t), &context);
}

static void end_input(ecs_iter_t *iter)
{
	nkui_context_t *context = ecs_field(iter, nkui_context_t, 0);
	nk_input_end(&context->nk);
}

static void begin_input(ecs_iter_t *iter)
{
	nkui_context_t *context = ecs_field(iter, nkui_context_t, 0);
	nk_input_begin(&context->nk);
}

void ecs_add_nkui()
{
	ecs_observer_init(ecs_world(), &(ecs_observer_desc_t){
		.query.terms = {
			(ecs_term_t){.id = EcsWindow, .inout = EcsIn},
			(ecs_term_t){.id = EcsGpuDevice, .inout = EcsIn},
		},
		.events = {EcsOnSet},
		.callback = init,
	});

	ecs_system_init(ecs_world(), &(ecs_system_desc_t){
		.entity = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
			.name = "NkEndInput",
			.add = ecs_ids(ecs_dependson(ecs_phase(PHASE_UPDATE_BEGIN))),
		}),
		.query.terms = {
			(ecs_term_t){.id = EcsNkContext, .inout = EcsInOut},
		},
		.callback = end_input,
	});

	ecs_system_init(ecs_world(), &(ecs_system_desc_t){
		.entity = ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
			.name = "NkBeginInput",
			.add = ecs_ids(ecs_dependson(ecs_phase(PHASE_UPDATE_END))),
		}),
		.query.terms = {
			(ecs_term_t){.id = EcsNkContext, .inout = EcsInOut},
		},
		.callback = begin_input,
	});
}
