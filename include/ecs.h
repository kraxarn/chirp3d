#pragma once

typedef struct ecs_world_t ecs_world_t;

void ecs_create();

void ecs_destroy();

[[nodiscard]]
ecs_world_t *ecs_world();

[[nodiscard]]
const void *ecs_const_data(const char *name);

[[nodiscard]]
void *ecs_mut_data(const char *name);
