#include "physicsconfig.h"

physics_config_t physics_config_create_default()
{
	return (physics_config_t){
		.move_speed = 250.F,
		.max_move_speed = 50.F,
		.gravity_y = 100.F,
		.jump_speed = 60.F,
	};
}
