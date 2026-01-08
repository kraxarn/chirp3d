#pragma once

typedef struct physics_engine_t physics_engine_t;

[[nodiscard]]
physics_engine_t *physics_engine_create();

void physics_engine_destroy(physics_engine_t *engine);
