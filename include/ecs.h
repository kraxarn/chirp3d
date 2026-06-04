#pragma once

typedef struct ecs_world_t ecs_world_t;

ecs_world_t *ecs_create();

void ecs_destroy(ecs_world_t *world);
