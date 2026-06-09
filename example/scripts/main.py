import math
import ecs


@ecs.component
class Position(math.Vector2):
	pass


@ecs.component
class Velocity(math.Vector2):
	pass


@ecs.system(ecs.Phase.ON_UPDATE, "Position, [in] Velocity")
def move(it: ecs.Iterator):
	pos = it.field(Position, 0)
	vel = it.field(Velocity, 1)
	pos += vel
	print("positon:", pos, "velocity:", vel)


if __name__ == "__main__":
	ecs.spawn(Position(10, 20), Velocity(1, 2))
