#pragma once

typedef struct physics_config_t
{
	float move_speed;
	float max_move_speed;
	float gravity_y;
	float jump_speed;
} physics_config_t;

[[nodiscard]]
physics_config_t physics_config_create_default();
