#include "prefabs.h"
#include "ecs.h"
#include "ecs/components.h"

#include "flecs.h"

ecs_entity_t prefab_model_instance(const char *name)
{
	return ecs_entity_init(ecs_world(), &(ecs_entity_desc_t){
		.set = (ecs_value_t[]){
			(ecs_value_t){
				.type = EcsModelInstance,
				.ptr = &(model_instance_t){
					.name = SDL_strdup(name), // TODO: Memory leak
				},
			},
			ecs_values_end,
		},
	});
}
