#include "physicsconfig.h"

physics_config_t physics_config_create_default()
{
	return (physics_config_t){
		.move_speed = 150.F,
		.max_move_speed = 7.5F,
		.gravity_y = 10.F,
		.jump_speed = 5.F,
	};
}
